#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "ChatSession.h"

bool Room::Enter(PlayerRef player)
{
	WRITE_LOCK;

	// 1. 방이 가득 찼는지 확인합니다.
	if (_players.size() >= 10)
	{
		// 2. 가득 찼다면 false를 반환하여 입장에 실패했음을 알립니다.
		return false;
	}

	// 3. 플레이어를 방에 추가합니다.
	_players[player->playerId] = player;
	player->_room = shared_from_this();

	// 4. 성공적으로 입장했으므로 true를 반환합니다.
	return true;
}

void Room::Leave(PlayerRef player)
{
	WRITE_LOCK;
	_players.erase(player->playerId);
	player->_room = nullptr; // 플레이어의 방 정보도 초기화합니다.
}

void Room::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (auto& p : _players)
	{
		p.second->ownerSession->Send(sendBuffer);
	}
}

// 특정 플레이어를 제외하고 모든 플레이어에게 패킷을 브로드캐스트합니다.
void Room::BroadcastWithoutSelf(SendBufferRef sendBuffer, uint64 selfId)
{
	WRITE_LOCK;
	for (auto& pair : _players)
	{
		if (pair.first != selfId)
		{
			pair.second->ownerSession->Send(sendBuffer);
		}
	}
}
