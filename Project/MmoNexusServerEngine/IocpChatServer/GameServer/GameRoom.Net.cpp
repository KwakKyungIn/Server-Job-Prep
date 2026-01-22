#include "pch.h"
#include "GameRoom.Net.h"
#include "Player.h"
#include "PlayerSession.h"
#include "GameSessionManager.h"

std::uint64_t NetId(const std::shared_ptr<Creature>& c)
{
    if (!c) return 0;

    if (c->GetObjectType() == Protocol::OBJECT_TYPE_PLAYER)
        return static_cast<std::uint64_t>(std::static_pointer_cast<Player>(c)->GetPlayerId());

    return static_cast<std::uint64_t>(c->GetObjectId()); // 몬스터/투사체 등
}

std::shared_ptr<PlayerSession> FindSessionByPlayerId(std::uint64_t playerId)
{
    if (!GameSessionManager::GSessionManager)
        return nullptr;

    return GameSessionManager::GSessionManager->FindByPlayerId(static_cast<uint64>(playerId));
}

void SendToPlayer(std::uint64_t playerId, const std::shared_ptr<SendBuffer>& sb)
{
    if (!sb) return;

    if (auto s = FindSessionByPlayerId(playerId))
        s->Send(sb);
}
