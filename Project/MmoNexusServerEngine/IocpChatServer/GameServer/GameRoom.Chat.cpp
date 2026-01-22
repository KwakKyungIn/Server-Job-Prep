#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"

void GameRoom::BroadcastChat(const Protocol::S_CHAT_NTF& ntf)
{
	// GameRoom은 JobQueue로 직렬 실행되는 전제라 별도 락 없이 간다.
	for (auto it = _players.begin(); it != _players.end(); ++it)
	{
		PlayerRef player = it->second;
		if (!player) continue;

		Protocol::S_CHAT_NTF pkt;
		pkt.CopyFrom(ntf);

		auto sb = ClientPacketHandler::MakeSendBuffer(pkt);
		SendToPlayer(player->GetPlayerId(), sb);
	}
}

void GameRoom::HandleChatById(PlayerSessionRef session, uint64 playerId, const std::string& msg)
{
	auto it = _players.find(playerId);
	if (it == _players.end())
		return;

	PlayerRef player = it->second;
	if (!player) return;

	Protocol::S_CHAT_NTF ntf;
	ntf.set_playerid(player->GetPlayerId());
	ntf.set_name(player->GetName());
	ntf.set_message(msg);

	BroadcastChat(ntf);
}
