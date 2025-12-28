#pragma once

// forward declarations (°¡º±°Ô)
class Creature;
class PlayerSession;
class SendBuffer;

// util
std::uint64_t NetId(const std::shared_ptr<Creature>& c);
std::shared_ptr<PlayerSession> FindSessionByPlayerId(std::uint64_t playerId);
void SendToPlayer(std::uint64_t playerId, const std::shared_ptr<SendBuffer>& sb);
