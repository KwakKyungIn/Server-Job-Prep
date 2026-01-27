#pragma once
#include "pch.h"
#include "Types.h"
#include <memory>

class Player;
class Monster;
class Projectile;

// 스마트 포인터 별칭 정의
// 헤더 의존성 줄이려고 전방 선언 사용함
using PlayerRef = std::shared_ptr<Player>;
using MonsterRef = std::shared_ptr<Monster>;
using ProjectileRef = std::shared_ptr<Projectile>;

// [공간 분할의 최소 단위]
// 넓은 맵을 바둑판처럼 잘게 쪼갰을 때, 그 한 칸(Cell)을 의미하는 구조체
// 나랑 같은 Zone, 그리고 인접한 Zone에 있는 애들만 체크하려고 만듦
struct Zone
{
    Set<PlayerRef>  players;
    Set<MonsterRef> monsters;

    // 투사체는 수명이 짧고 엄청 자주 생겼다 사라짐
    // 몬스터나 플레이어랑 섞어두면 관리하기 힘들어서 따로 뺌
    Set<ProjectileRef> projectiles;
};