// GameContents/AOI/Zone.h
#pragma once
#include "pch.h"
#include "Types.h"
#include <memory>

class Player;
class Monster;

using PlayerRef = std::shared_ptr<Player>;
using MonsterRef = std::shared_ptr<Monster>;

// 하나의 셀(Zone)에 들어있는 객체들
struct Zone
{
    Set<PlayerRef>  players;
    Set<MonsterRef> monsters;
};
