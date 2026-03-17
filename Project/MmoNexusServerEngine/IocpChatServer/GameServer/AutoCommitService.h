#pragma once
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <functional>
#include "Protocol_S2S.pb.h"

class RedisManager;

namespace Persistence
{
    // 주기적으로 DB에 데이터를 저장하기 위한 서비스 클래스
    // 메인 로직과 분리된 별도의 스레드에서 돌아가게 설계함
    class AutoCommitService
    {
    public:
        // 저장 패킷을 실제로 서버로 쏘는 콜백 함수들 정의
        using SendCoreFn = std::function<void(const Protocol::S2S_REQ_SAVE_PLAYER_CORE&)>;
        using SendInvFn = std::function<void(const Protocol::S2S_REQ_SAVE_INVENTORY&)>;

        // 싱글톤 패턴으로 접근
        static AutoCommitService& I();

        // 초기화 함수
        // Redis 매니저랑 패킷 전송 콜백을 여기서 연결해준다
        void Init(RedisManager* redis, SendCoreFn sendCore, SendInvFn sendInv);

        // 스레드 시작 및 종료
        void Start();
        void Stop();

        // 로그아웃하거나 연결 끊겼을 때 즉시 저장해야 할 경우 호출
        // 이걸 호출하면 다음 틱을 기다리지 않고 바로 저장 로직을 수행함
        void RequestFlushNow(uint64 pid);

        // Persistence Drain baseline에서 QuickSlot 변경을 즉시 저장할 때 사용
        bool SendQuickSlotImmediate(uint64 pid);

        // DB 서버로부터 저장 완료 응답이 왔을 때 호출됨
        // 진행 중인 저장 카운트를 줄여서 다음 저장이 가능하게 만듦
        void OnCommitFinished(uint64 pid);

        // 스레드 없이 테스트 목적으로 1회만 강제로 실행해볼 때 사용
        void TickOnce();

        using SendQsFn = std::function<void(const Protocol::S2S_REQ_SAVE_QUICKSLOT&)>;

        // 퀵슬롯 저장 기능이 추가되면서 만든 오버로딩 초기화 함수
        void Init(RedisManager* redis, SendCoreFn sendCore, SendInvFn sendInv, SendQsFn sendQs);

    private:
        SendQsFn _sendQs;

        // 현재 저장 요청이 날아가서 응답 대기 중인 플레이어 목록 관리
        // 중복 저장을 막기 위해 카운팅으로 관리함
        HashMap<uint64, int32> _inflightCount;


    private:
        AutoCommitService() = default;

        // 백그라운드 스레드가 실제로 돌게 될 루프 함수
        void WorkerLoop();

        // 실제 저장 로직이 들어있는 핵심 함수
        // Redis에서 Dirty Flag 체크하고 스냅샷 떠서 패킷 보냄
        void TickCommit_Internal();

        // Redis에서 문자열로 넘어온 PID를 숫자로 변환
        bool ParsePid(const std::string& s, uint64& outPid);

    private:
        RedisManager* _redis = nullptr;
        SendCoreFn _sendCore;
        SendInvFn  _sendInv;

        std::thread _worker;
        std::atomic<bool> _running{ false };

        // 스레드 동기화를 위한 뮤텍스와 조건변수
        std::mutex _mx;
        std::condition_variable _cv;

        // 즉시 저장 요청된 대상들 목록
        HashSet<uint64> _flushNow;
        HashSet<uint64> _inflight;
    };
}
