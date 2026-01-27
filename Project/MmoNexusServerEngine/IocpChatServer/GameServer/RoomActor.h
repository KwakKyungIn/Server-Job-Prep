// RoomActor.h
#pragma once
#include <functional>
#include <memory>

// 룸 타입을 구분하기 위한 열거형
// int 대신 enum class를 써서 타입 안정성을 높임
enum class RoomKind : uint8_t
{
    Lobby = 0, // 캐릭터 선택창 혹은 대기실
    Game = 1,  // 실제 게임 필드
};

// 룸의 최상위 부모 클래스
// LobbyRoom이랑 GameRoom이 하는 일이 달라도, "Job을 받아서 처리한다"는 Actor의 본질은 같음
// 다형성을 활용해서 RoomManager가 통합 관리할 수 있게 설계함
class RoomActor
{
public:
    virtual ~RoomActor() = default;

    virtual RoomKind GetKind() const = 0;

    // Actor 모델의 핵심
    // 외부에서는 이 함수를 통해 일감(람다)을 던져주고, 실제 실행은 룸이 가진 스레드(JobQueue)가 처리함
    // 이렇게 해야 룸 내부 데이터에 대한 락을 최소화할 수 있음
    virtual void Push(std::function<void()> fn) = 0;
};

using RoomActorRef = std::shared_ptr<RoomActor>;
using RoomActorWeakRef = std::weak_ptr<RoomActor>;