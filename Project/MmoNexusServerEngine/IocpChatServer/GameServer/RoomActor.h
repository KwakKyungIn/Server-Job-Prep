// RoomActor.h
#pragma once
#include <functional>
#include <memory>

enum class RoomKind : uint8_t
{
    Lobby = 0,
    Game = 1,
};

class RoomActor
{
public:
    virtual ~RoomActor() = default;

    virtual RoomKind GetKind() const = 0;

    // Actor 실행(잡큐에 태우는) 공통 API
    virtual void Push(std::function<void()> fn) = 0;
};

using RoomActorRef = std::shared_ptr<RoomActor>;
using RoomActorWeakRef = std::weak_ptr<RoomActor>;
