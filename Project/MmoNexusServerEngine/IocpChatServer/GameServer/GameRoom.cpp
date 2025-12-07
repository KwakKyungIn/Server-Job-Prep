#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h" 

GameRoom::GameRoom()
{
	_jobQueue = MakeShared<JobQueue>();
}

GameRoom::~GameRoom()
{
}

void GameRoom::Init(int32 mapId, int32 sizeX, int32 sizeY)
{
	_map = MakeShared<GameMap>();
	_map->Init(mapId, sizeX, sizeY);
}

void GameRoom::Update()
{
	// TODO: 몬스터 업데이트 등
}

void GameRoom::Enter(PlayerSessionRef session)
{
	// [Check] 이미 들어와 있는지 확인
	if (_sessions.find(session->GetPlayerId()) != _sessions.end())
		return;

	// 1. 룸 연결 및 명단 추가
	session->SetRoom(shared_from_this());
	_sessions.insert({ session->GetPlayerId(), session });

	printf("[ROOM] Player %llu Entered. Total Users: %d\n", session->GetPlayerId(), (int)_sessions.size());

	// 2. [To Others] 기존 유저들에게 "내가 왔다"고 알림
	{
		Protocol::S_SPAWN spawnPkt;
		Protocol::PlayerInfo* pInfo = spawnPkt.add_players();
		*pInfo = *session->GetPlayerInfo();

		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);
		Broadcast(sendBuffer, session->GetPlayerId());
	}

	// 3. [To Me] 나에게 "기존 유저들 정보" 알림
	{
		Protocol::S_SPAWN spawnPkt;
		for (auto& item : _sessions)
		{
			PlayerSessionRef other = item.second;
			if (other == session) continue;

			Protocol::PlayerInfo* pInfo = spawnPkt.add_players();
			*pInfo = *other->GetPlayerInfo();
		}

		// [Optimization] 보낼 유저가 있을 때만 전송
		if (spawnPkt.players_size() > 0)
		{
			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);
			session->Send(sendBuffer);

			// [Log Fix] 루프 밖에서 한 번만 출력
			printf("[ROOM] Player %llu received list of %d existing players.\n", session->GetPlayerId(), spawnPkt.players_size());
		}
	}
}

void GameRoom::Leave(PlayerSessionRef session)
{
	if (_sessions.find(session->GetPlayerId()) == _sessions.end())
		return;

	// 1. 명단 제거 및 룸 연결 해제
	_sessions.erase(session->GetPlayerId());
	session->SetRoom(nullptr);

	printf("[ROOM] Player %llu Left. Total Users: %d\n", session->GetPlayerId(), (int)_sessions.size());

	// 2. [To Others] 남은 사람들에게 "나 나갔다"고 알림
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_playerids(session->GetPlayerId());

		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);
		Broadcast(sendBuffer);
	}
}

void GameRoom::HandleMove(PlayerSessionRef session, Protocol::C_MOVE pkt)
{
	// 1. [Validation] 갈 수 있는 곳인가?
	if (_map->CanGo(pkt.posinfo()) == false)
	{
		return; // TODO: 롤백 패킷 전송
	}

	// 2. [Update] 서버 좌표 갱신
	session->SetPosInfo(pkt.posinfo());

	// 3. [Broadcast] 주변 전파
	{
		Protocol::S_MOVE movePkt;
		movePkt.set_playerid(session->GetPlayerId());
		*movePkt.mutable_posinfo() = pkt.posinfo();

		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);
		Broadcast(sendBuffer, session->GetPlayerId());
	}

	// 이동 로그는 너무 많으므로 주석 처리하거나 필요할 때만 켬
	// printf("[ROOM] Player %llu Moved.\n", session->GetPlayerId());
}

void GameRoom::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	for (auto& item : _sessions)
	{
		PlayerSessionRef session = item.second;
		if (session->GetPlayerId() == exceptId) continue;

		session->Send(sendBuffer);
	}
}