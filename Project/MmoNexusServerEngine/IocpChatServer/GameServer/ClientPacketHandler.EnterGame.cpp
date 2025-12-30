#include "pch.h"
#include "ClientPacketHandler.h"
#include "S2SPacketHandler.h" 
#include "PlayerSession.h"
#include "GameSessionManager.h"
#include "RedisManager.h"
#include "DataManager.h"
#include "LobbyRoom.h"

extern shared_ptr<PacketSession> G_DBSession;

bool ClientPacketHandler::Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	// 1) Redis 토큰 검증 (Blocking이지만 일단 유지)
	std::string token = pkt.token();
	std::string value = GRedisManager->Get(token);

	if (value.empty())
	{
		printf("❌ [EnterGame] Invalid Token: %s\n", token.c_str());
		ps->Disconnect(L"Invalid Token");
		return false;
	}

	uint64 playerId = std::stoull(value);

	int32 channelId = pkt.channelid();
	if (channelId <= 0) channelId = 1;

	int32 mapId = pkt.mapid();
	DataManager* dm = DataManager::Instance();

	// ✅ ENTER_GAME은 "월드맵만" 허용. 던전맵 요청이면 기본 월드맵으로 교정
	if (!dm)
	{
		mapId = 1;
	}
	else
	{
		if (!dm->IsValidMapId(mapId) || !dm->IsWorldMapId(mapId))
			mapId = dm->GetDefaultWorldMapId();

		// 방어: cfg가 없으면 기본 월드맵으로 한번 더 교정
		if (dm->GetMapConfig(mapId) == nullptr)
			mapId = dm->GetDefaultWorldMapId();
	}

	const MapConfig* cfg = (dm ? dm->GetMapConfig(mapId) : nullptr);


	// 2) spawn 계산까지 끝났다는 전제: cfg 기반 spawn 만들었지?
	Protocol::PositionInfo spawn;
	spawn.set_x(cfg ? cfg->spawnX : 50.f);
	spawn.set_y(cfg ? cfg->spawnY : 0.f);
	spawn.set_z(cfg ? cfg->spawnZ : 50.f);

	// 3) 이제부터는 Session이 Player를 들지 않는다.
	//    Session Actor에서: playerId 바인딩 + 로비에 “Player 생성/소유” 위임 + DB 요청
	ps->Post([playerId, channelId, mapId, spawn](PlayerSessionRef ps) mutable
		{
			GameSessionManager::GSessionManager->BindPlayerId(ps, playerId);
			ps->SetPlayerId_ActorOnly(playerId);
			if (GLobbyRoom)
			{
				GLobbyRoom->Push([ps, playerId, channelId, mapId, spawn]() mutable
					{
						GLobbyRoom->EnterGame(ps, playerId, channelId, mapId, spawn);
					});
			}

			if (G_DBSession)
			{
				Protocol::S2S_REQ_LOAD_PLAYER_DATA reqStat;
				reqStat.set_playerid(playerId);
				reqStat.set_gamesessionid(ps->GetSessionId());
				G_DBSession->Send(S2SPacketHandler::MakeSendBuffer(reqStat));

				Protocol::S2S_REQ_ITEMS_LOAD reqItem;
				reqItem.set_playerid(playerId);
				reqItem.set_gamesessionid(ps->GetSessionId());
				G_DBSession->Send(S2SPacketHandler::MakeSendBuffer(reqItem));
			}
		});


	return true;
}

bool ClientPacketHandler::Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
	return false;
}
