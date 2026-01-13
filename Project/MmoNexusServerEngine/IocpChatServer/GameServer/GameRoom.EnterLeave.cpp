#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "PlayerSession.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"
#include "Monster.h"
#include "PartyActor.h"
#include "Projectile.h"


bool GameRoom::EnterRegister(PlayerSessionRef session, PlayerRef player)
{
	printf("1번 여기가 문제임\n");
	if (player == nullptr) return false;

	if (IsClosing())
		return false;

	player->SetInstanceId(_instanceId);


	// 이미 들어와 있으면 실패 (중복 Enter 방지)
	if (_players.find(player->GetPlayerId()) != _players.end())
		return false;

	// 1) 룸 소속 설정
	player->SetRoom(shared_from_this());

	auto room = shared_from_this();
	session->Post([room](PlayerSessionRef ps)
		{
			ps->SetCurrentRoom(room);
		});

	_players.insert({ player->GetPlayerId(), player });


	// playerCount 증가 + emptySince 리셋
	_playerCount.fetch_add(1, std::memory_order_acq_rel);
	_emptySinceMs.store(0, std::memory_order_release);



	// 2) AOI Zone 계산 및 등록
	int32 zoneIndex = _grid.GetZoneIndex(*player->GetPosInfo());
	player->SetZoneIndex(zoneIndex);

	Zone& enterZone = _grid.GetZone(zoneIndex);
	enterZone.players.insert(player);

	printf("🎮 [EnterRegister] Player %llu Zone[%d] at (%.1f, %.1f, %.1f)\n",
		player->GetPlayerId(), zoneIndex,
		player->GetPosInfo()->x(),
		player->GetPosInfo()->y(),
		player->GetPosInfo()->z());

	return true;
}

void GameRoom::Enter(PlayerSessionRef session, PlayerRef player)
{
	if (!session || !player)
		return;

	if (EnterRegister(session, player) == false)
		return;


	if (player == nullptr) return;

	// 1) 응답 먼저
	Protocol::S_ENTER_GAME enterPkt;
	enterPkt.set_success(true);
	enterPkt.mutable_myplayer()->CopyFrom(*player->GetPlayerInfo());
	// NOTE: 네 proto에 mapid가 실제로 있으면 유지, 없으면 이 줄 삭제
	// enterPkt.set_mapid(_mapId);

	session->Send(ClientPacketHandler::MakeSendBuffer(enterPkt));

	// 2) 스폰 전송은 그 다음
	UpdateAOI(session, player, true /*forceFullSnapshot*/);

	printf("✅ [Enter-Login] Player %llu\n", player->GetPlayerId());
}

// [맵 이동 입장]
// [맵 이동 입장]
void GameRoom::EnterMapChange(PlayerSessionRef session, PlayerRef player)
{
	printf("2번 여기가 문제임\n");
	if (!session || !player)
		return;

	// ✅ EnterRegister 실패하면 MapChange 상태를 반드시 풀어줘야 한다.
	if (EnterRegister(session, player) == false)
	{
		session->Post([](PlayerSessionRef ps)
			{
				ps->CancelMapChange();
			});
		return;
	}

	const uint64 playerId = player->GetPlayerId();

	// 1) END 응답 먼저
	Protocol::S_MAP_CHANGE_END endPkt;
	endPkt.set_token(session->GetMapChangeToken());
	endPkt.set_mapid(_mapId);
	endPkt.mutable_pos()->CopyFrom(*player->GetPosInfo());
	endPkt.set_instanceid(_instanceId);     
	endPkt.set_targetchannelid(_channelId);       

	session->Send(ClientPacketHandler::MakeSendBuffer(endPkt));

	// ❌ 여기서 EndMapChange 하면 안 됨 (중복/조기해제/레이스)
	// session->EndMapChange();

	// 2) 스폰은 그 다음
	UpdateAOI(session, player, true /*forceFullSnapshot*/);

	// 3) 파티 정보 재전송
	PartyActor::Instance().Push([session, playerId]()
		{
			auto& core = PartyActor::Instance().Core();
			const uint64 partyId = core.GetPartyIdByPlayerId(playerId);

			if (partyId != 0)
			{
				auto snap = core.GetSnapshot(partyId);
				if (snap.partyId != 0)
				{
					Protocol::S_PARTY_INFO_NTF info;
					info.set_hasparty(true);
					info.set_partyid(snap.partyId);
					info.set_leaderid(snap.leaderId);
					info.set_version(snap.version);
					for (uint64 id : snap.members)
						info.add_memberids(id);

					session->Post([info](PlayerSessionRef s) mutable
						{
							s->Send(ClientPacketHandler::MakeSendBuffer(info));
						});
				}
			}
		});

	printf("✅ [MapChange-END] Player %llu -> Map %d (Inst=%lld) Token=%llu Channel=%d\n",
		playerId, _mapId, (long long)_instanceId, endPkt.token(), endPkt.targetchannelid());
}

void GameRoom::Leave(PlayerSessionRef session, PlayerRef player)
{
	if (!session || !player) return;

	const uint64 meId = player->GetPlayerId();
	auto itMe = _players.find(meId);
	if (itMe == _players.end()) return;

	// 1) 내가 보던 플레이어들에게 "나 despawn" + 상대 set에서 나 제거
	{
		Protocol::S_DESPAWN pkt;
		pkt.add_objectids(meId);
		SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);

		auto& visP = player->VisiblePlayers_ActorOnly();
		for (uint64 vid : visP)
		{
			PlayerRef other = FindPlayer_ActorOnly(vid);
			if (!other) continue;
			other->VisiblePlayers_ActorOnly().erase(meId);
			SendToPlayer(vid, sb);
		}
		visP.clear();

		// ✅ [추가] 내가 보던 몬스터들의 viewers에서 나 제거
		auto& visM = player->VisibleMonsters_ActorOnly();
		for (uint64 mid : visM)
		{
			auto it = _monsters.find(mid);
			if (it != _monsters.end() && it->second)
				it->second->Viewers_ActorOnly().erase(meId);
		}
		visM.clear();

		auto& visPr = player->VisibleProjectiles_ActorOnly();
		for (uint64 prid : visPr)
		{
			auto it = _projectiles.find(prid);
			if (it != _projectiles.end() && it->second)
				it->second->Viewers_ActorOnly().erase(meId);
		}
		visPr.clear();
	}

	// 2) grid에서 제거
	int32 zoneIndex = player->GetZoneIndex();
	int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
	if (zoneIndex >= 0 && zoneIndex < totalZones)
		_grid.GetZone(zoneIndex).players.erase(player);

	// 3) 맵에서 제거
	_players.erase(meId);
	player->SetRoom(nullptr);

	auto room = shared_from_this();
	session->Post([room](PlayerSessionRef ps) { ps->ClearCurrentRoom(room); });

	const int32 after = _playerCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
	if (after == 0)
		_emptySinceMs.store(::GetTickCount64(), std::memory_order_release);
}

void GameRoom::LeaveById(PlayerSessionRef session, uint64 playerId)
{
	auto it = _players.find(playerId);
	if (it == _players.end())
		return;

	PlayerRef player = it->second;

	// 기존 Leave 로직 재사용(지금 단계에선 이게 제일 빠름)
	Leave(session, player);
}

PlayerRef GameRoom::FindPlayer_ActorOnly(uint64 playerId) const
{
	auto it = _players.find(playerId);
	if (it == _players.end())
		return nullptr;
	return it->second;
}
