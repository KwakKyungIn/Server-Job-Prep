#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"

void GameRoom::BroadcastChat(const Protocol::S_CHAT_NTF& ntf)
{
	// 채팅 패킷을 방에 있는 모든 유저에게 뿌려주는 함수
	// GameRoom 로직은 JobQueue(싱글 스레드)로 돌기 때문에 별도의 Lock 없이 순회 가능
	for (auto it = _players.begin(); it != _players.end(); ++it)
	{
		PlayerRef player = it->second;
		if (!player) continue;

		// Protobuf 객체 복사 비용이 좀 들긴 하지만, 안전하게 처리
		Protocol::S_CHAT_NTF pkt;
		pkt.CopyFrom(ntf);

		auto sb = ClientPacketHandler::MakeSendBuffer(pkt);
		SendToPlayer(player->GetPlayerId(), sb);
	}
}

void GameRoom::HandleChatById(PlayerSessionRef session, uint64 playerId, const std::string& msg)
{
	// 특정 플레이어가 채팅을 쳤을 때 호출됨
	auto it = _players.find(playerId);
	if (it == _players.end())
		return;

	PlayerRef player = it->second;
	if (!player) return;

	// 채팅 패킷 조립 (누가, 뭐라고 말했는지)
	Protocol::S_CHAT_NTF ntf;
	ntf.set_playerid(player->GetPlayerId());
	ntf.set_name(player->GetName());
	ntf.set_message(msg);

	BroadcastChat(ntf);
}