#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "Player.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "Monster.h"
#include "DataManager.h"
#include "ObjectUtils.h"
#include "BattleSystem.h"
#include "Zone.h"
#include "Creature.h"
#include "GameSessionManager.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"

void GameRoom::HandleSkill(std::shared_ptr<Creature> attacker, int32 skillId)
{
	if (attacker == nullptr)
		return;

	// 방 검증
	if (attacker->GetRoom().get() != this)
		return;

	if (_battle == nullptr)
		return;

	// 1. BattleSystem에 전투 판정 위임
	SkillResult result;
	if (_battle->ResolveSkill(attacker, skillId, result) == false)
		return;

	// 2. 스킬 모션 브로드캐스트
	{
		Protocol::S_SKILL skillPkt;
		skillPkt.set_objectid(NetId(attacker));
		skillPkt.set_skillid(skillId);

		SendBufferRef skillBuffer = ClientPacketHandler::MakeSendBuffer(skillPkt);
		BroadcastToZone(skillBuffer, result.zoneIndex);
	}

	// 3. 피격 결과 브로드캐스트 (HP 변경)
	for (const HitInfo& hit : result.hits)
	{
		auto victim = hit.target;
		if (victim == nullptr) continue;

		Protocol::S_CHANGE_HP changePkt;
		changePkt.set_objectid(NetId(victim));
		changePkt.set_attackerid(NetId(attacker));
		changePkt.set_currenthp(victim->GetStatInfo()->hp()); // OnDamaged 후 HP
		changePkt.set_damage(hit.damage);

		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(changePkt);
		BroadcastToZone(sendBuffer, result.zoneIndex);
	}
}

void GameRoom::HandleSkillById(PlayerSessionRef session, uint64 playerId, int32 skillId)
{
	auto it = _players.find(playerId);
	if (it == _players.end())
		return;

	PlayerRef player = it->second;
	if (!player) return;

	std::shared_ptr<Creature> attacker = std::static_pointer_cast<Creature>(player);
	HandleSkill(attacker, skillId);
}
