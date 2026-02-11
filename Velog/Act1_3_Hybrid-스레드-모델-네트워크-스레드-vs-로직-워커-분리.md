# 네트워크 스레드가 로직에 잠기지 않게 한 Hybrid 스레드 모델 — IOCP Dispatch와 GlobalQueue/JobQueue 분리

## 요약하자면
- 무엇을 해결: IOCP 완료 통지 처리(네트워크)와 게임/DB 로직 처리가 같은 스레드에서 서로 잡아먹는 문제를 줄였습니다.
- 어떻게 해결: 네트워크 스레드는 `core->Dispatch(10)`만 수행하고, 로직은 `JobQueue -> GlobalQueue -> DoGlobalQueueWork()` 경로로 분리했습니다.
- 결과/효과: 패킷 진입 경로가 가벼워졌고, 룸/파티/인스턴스/DB 작업을 액터 큐에서 직렬 처리하는 기준이 명확해졌습니다.

---

## 1. 필요했던 이유
핵심은 "네트워크 스레드는 빨라야 하고, 로직은 안전해야 한다"였습니다.

IOCP 서버에서 `Dispatch`는 계속 돌아야 합니다. 그런데 이 스레드에서 무거운 로직(특히 DB 쿼리, 복잡한 룸 처리)을 같이 돌리면 아래 문제가 생깁니다.

- 완료 통지 처리 지연: Recv/Send/Accept 반응성이 떨어짐
- 경쟁 조건 증가: 세션/룸/파티 상태를 여러 스레드가 건드리기 쉬움
- 원인 추적 어려움: 네트워크 병목인지 로직 병목인지 구분이 어려움

그래서 구조를 아래 원칙으로 정리했습니다.

- 네트워크 스레드: 완료 통지 수집/분배만 담당
- 로직 스레드: 큐에 쌓인 작업만 순차 실행
- 컨텐츠 객체(Room, Party, Instance, Session): 액터 큐로 직렬화

---

## 2. 첫 시도와 실패
초기에는 패킷 핸들러에서 로직/DB를 즉시 처리하는 방식에 가까웠습니다.

```cpp
// 초기 형태(의도): 패킷 핸들러에서 즉시 무거운 작업 수행
bool HandleReq(PacketSessionRef& session, Req& pkt)
{
    DBConnection* conn = pool->Pop();
    conn->Execute(); // 블로킹 구간
    session->Send(...);
    return true;
}
```

이 방식은 구현은 단순하지만, IOCP 관점에서는 좋지 않았습니다.

- 네트워크 경로에 블로킹 작업이 섞여 `Dispatch`가 밀림
- 패킷 처리량이 늘수록 tail-latency가 커짐
- 상태 변경 지점이 분산되어 동시성 오류 재현이 어려움

현재 DBAgent 코드에 `PushJob` 래핑 주석이 남아 있는 이유도, 이 경계를 분리한 흔적입니다.

---

## 3. 어떻게 풀었나
핵심은 "진입은 빠르게, 실행은 큐에서"로 실행 경로를 고정한 것입니다.

1. 네트워크 스레드는 `Dispatch(10)` 루프만 돈다.
2. 핸들러는 일을 직접 처리하지 않고 대상 액터의 `JobQueue`에 밀어 넣는다.
3. `JobQueue`는 최초 등록 시에만 `GlobalQueue`에 게시한다(`_posted` 플래그).
4. 로직 워커(`DoGlobalQueueWork`)가 `GlobalQueue`에서 큐를 꺼내 `Execute()`로 소모한다.

### 3.1 흐름 요약 다이어그램
```text
[Network Worker Threads]
  while (running) core->Dispatch(10)
           │
           ▼
   Packet Handler Entry
           │
           ├─ session/lobby/room/party/instance -> JobQueue::Push(job)
           ▼
      GlobalQueue::Push(jobQueue) (최초 게시 시)
           ▼
[Logic Worker Threads]
  ThreadManager::DoGlobalQueueWork()
           │
           ▼
      jobQueue->Execute()
           ▼
   RoomActor / PartyActor / InstanceActor 로직 실행
```

---

## 4. 구현의 핵심
핵심 구현은 아래 파일에서 확인할 수 있습니다.

- `GameServer/GameServer.cpp` — 네트워크/로직 스레드 분리 기동
- `ServerCore/ThreadManager.cpp` — 글로벌 큐 워커 루프
- `ServerCore/JobQueue.cpp` — `_posted` 기반 단일 게시 + 실행
- `ServerCore/GlobalQueue.cpp` — 32-shard 큐 + work stealing
- `GameServer/RoomManager.cpp` — 룸 업데이트를 직접 실행하지 않고 큐로 위임
- `DBAgent/DBAgentPacketHandler.cpp` — DB 작업을 `PushJob`으로 분리

스니펫 #1: 서버 시작 시 네트워크/로직 워커를 분리해 기동합니다.
```cpp
int32 threadCount = std::thread::hardware_concurrency();
if (threadCount < 2) threadCount = 2;

int32 networkThreadCount = threadCount / 2;
int32 logicThreadCount = threadCount - networkThreadCount;

for (int32 i = 0; i < networkThreadCount; i++)
{
    GThreadManager->Launch([=]() {
        while (GIsRunning) { core->Dispatch(10); }
    });
}

for (int32 i = 0; i < logicThreadCount; i++)
{
    GThreadManager->Launch([=]() {
        ThreadManager::DoGlobalQueueWork();
    });
}
```

스니펫 #2: 로직 워커는 글로벌 큐에서 작업 큐를 꺼내 실행합니다.
```cpp
void ThreadManager::DoGlobalQueueWork()
{
    while (true)
    {
        if (GGlobalQueue == nullptr)
        {
            this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        shared_ptr<JobQueue> jobQueue = GGlobalQueue->Pop();
        if (jobQueue == nullptr)
        {
            this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        jobQueue->Execute();
    }
}
```

스니펫 #3: `JobQueue`는 최초 등록 시에만 글로벌 큐에 게시해 중복 등록을 줄입니다.
```cpp
void JobQueue::Push(shared_ptr<Job> job)
{
    const bool prev = _posted.exchange(true);

    {
        WRITE_LOCK;
        _jobs.push(job);
    }

    if (prev == false)
        GGlobalQueue->Push(shared_from_this());
}
```

스니펫 #4: 실행 루프는 큐를 한 번에 비우고, 레이스를 재확인한 뒤 종료합니다.
```cpp
void JobQueue::Execute()
{
    while (true)
    {
        vector<shared_ptr<Job>> jobs;
        {
            WRITE_LOCK;
            while (_jobs.empty() == false)
            {
                jobs.push_back(_jobs.front());
                _jobs.pop();
            }
        }

        if (jobs.empty())
        {
            _posted.store(false);
            {
                WRITE_LOCK;
                if (_jobs.empty())
                    break;

                const bool prev = _posted.exchange(true);
                if (prev == false)
                    continue;
                else
                    break;
            }
        }

        for (shared_ptr<Job>& job : jobs)
            job->Execute();
    }
}
```

스니펫 #5: 메인 루프는 룸 로직을 직접 실행하지 않고 각 룸 큐로 위임합니다.
```cpp
void RoomManager::UpdateAll()
{
    Vector<std::shared_ptr<GameRoom>> roomsCopy;

    {
        READ_LOCK;
        for (auto& kv : _rooms)
            roomsCopy.push_back(kv.second);
    }

    for (auto& room : roomsCopy)
    {
        if (!room) continue;
        room->Push([room]() { room->Update(); });
    }
}
```

스니펫 #6: DBAgent는 DB 쿼리를 세션 잡큐로 넘겨 네트워크 경로와 분리합니다.
```cpp
bool DBAgentPacketHandler::Handle_S2S_REQ_LOGIN(PacketSessionRef& session, Protocol::S2S_REQ_LOGIN& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    gameSession->PushJob(ObjectPool<Job>::MakeShared([gameSession, pkt]()
    {
        DBConnection* conn = GDBConnectionPool->Pop();
        if (conn == nullptr) return;

        // ... Prepare / Execute / Fetch ...

        GDBConnectionPool->Push(conn);
        gameSession->Send(DBAgentPacketHandler::MakeSendBuffer(resPkt));
    }));

    return true;
}
```

---

## 5. 경계 조건과 실패 케이스
"언제 발생하는가 -> 현재 어떻게 처리하는가" 순서로 정리합니다.

- 글로벌 큐가 비었을 때
  - 발생: 처리할 잡이 없는 유휴 구간
  - 처리: `DoGlobalQueueWork()`에서 10ms sleep 후 재시도해 busy-wait를 줄입니다.

- 같은 `JobQueue`가 여러 번 글로벌 큐에 중복 등록되는 문제
  - 발생: 짧은 시간에 `Push`가 몰릴 때
  - 처리: `_posted.exchange(true)`로 최초 게시만 허용하고, `Execute` 종료 시점에 재검증합니다.

- 락 경합
  - 발생: 단일 글로벌 큐에 push/pop이 몰릴 때
  - 처리: `GlobalQueue`를 32개 shard로 나누고, `Pop` 시 TLS 기반 우선 + stealing 순회 전략을 사용합니다.

- 세션/룸 상태 동시 변경
  - 발생: 핸들러에서 직접 룸 상태를 만지는 경우
  - 처리: `PostRoom`, `room->Push`, `PartyActor::Push`, `InstanceActor::Push`로 액터 큐 직렬화합니다.

- 현재 남은 리스크
  - `ThreadManager::DoGlobalQueueWork()`는 현재 `while(true)`라 명시적 종료 조건이 없습니다.
  - 종료 플래그(`GIsRunning`)를 로직 워커 루프에 반영하지 않으면 종료 시나리오가 깔끔하지 않을 수 있습니다.
  - `JobQueue` 백로그 길이 자체의 계측/제한이 아직 약해, 폭주 시 관측 지표가 부족합니다.

---

## 6. 트레이드오프

- 대안 1: 네트워크 스레드에서 바로 로직 수행
  - 선택: 네트워크/로직 분리
  - 이유: Dispatch 경로를 가볍게 유지하고, 로직 병목이 네트워크 반응성을 직접 해치지 않게 했습니다.

- 대안 2: 전역 단일 큐
  - 선택: 32-shard `GlobalQueue`
  - 이유: push/pop 경합을 분산하고 워커 locality를 높이기 위해서입니다.

- 대안 3: 공유 상태에 세밀한 락 추가
  - 선택: 액터 큐 기반 직렬화
  - 이유: 락 설계 복잡도를 줄이고, 상태 변경 순서를 명확히 하기 위해서입니다.

- 대안 4: 즉시 처리로 지연 최소화
  - 선택: 큐 위임으로 정합성 우선
  - 이유: 컨텍스트 전환 비용은 늘지만, 유지보수와 디버깅 안정성이 좋아졌습니다.

---

## 7. 테스트/측정
- 재현 시나리오
  - 동시 접속 + 이동/스킬/채팅/거래 요청을 함께 발생시켜 네트워크/로직 분리 효과 확인
  - DBAgent에 로그인/로딩 요청을 연속으로 넣어 네트워크 스레드 블로킹 여부 확인
  - 룸 수를 늘려 `RoomManager::UpdateAll -> room->Push` 경로의 누락/지연 확인

- 확인 포인트
  - 네트워크 스레드가 `Dispatch` 루프에서 장시간 막히지 않는지
  - 로직이 액터 큐 경로로만 들어가는지
  - `_posted` 동작으로 `JobQueue` 중복 게시가 억제되는지

- 관찰 포인트
  - 현재 코드에는 `GlobalQueue depth`, `JobQueue length`, `worker utilization` 같은 런타임 지표가 부족합니다.

- 수치
  - 아직 정량 지표는 없습니다.
  - 다음 단계에서 큐 길이/실행 시간/드롭률을 카운터화할 계획입니다.

---

## 8. 회고와 다음 편
- 배운 점
  - IOCP 서버에서 성능을 올리려면 "더 많은 스레드"보다 "역할 분리"가 먼저였습니다.
  - 특히 로직을 액터 큐로 고정하니, 코드가 길어져도 동시성 원인 추적이 쉬워졌습니다.

- 다음 개선
  - 로직 워커 종료 조건(`GIsRunning`)을 명시적으로 반영
  - 큐 백로그/처리 시간 지표를 수집해 병목 지점 시각화
  - 워커 수를 고정 반반이 아니라 부하 기반 동적 조절로 확장

- 다음 편 예고
  - 다음 글에서는 JobQueue/GlobalQueue 설계를 더 깊게 다룹니다.
  - 특히 `_posted` 플래그, shard queue, work stealing이 실제 경합을 어떻게 줄였는지 케이스 중심으로 정리하겠습니다.
