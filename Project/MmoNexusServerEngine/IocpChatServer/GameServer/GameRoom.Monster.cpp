#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "Monster.h"
#include "ExperimentUtils.h"
#include "GameMetrics.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"

void GameRoom::Update()
{
	// 현재 시간 체크해서 델타타임 계산
	// 서버 프레임 튀는 거 방지하려고 최대 100ms로 제한 둠
	const uint64 now = ::GetTickCount64();
	if (_lastUpdateMs == 0)
		_lastUpdateMs = now;

	uint64 deltaMs = now - _lastUpdateMs;
	if (deltaMs > 100) deltaMs = 100;
	_lastUpdateMs = now;

	// 방에 있는 모든 몬스터 AI 업데이트 돌림
	for (auto& item : _monsters)
		item.second->Update(now, deltaMs);

	// 투사체 이동 처리
	UpdateProjectiles(deltaMs);

	// 스폰/리스폰 처리
	UpdateSpawns_ActorOnly(now);

	// 거래 타임아웃 같은 거 체크
	UpdateTrades_ActorOnly(now);
}

void GameRoom::EnterMonster(MonsterRef monster)
{
	if (!monster) return;
	const uint64 mid = monster->GetObjectId();

	// 중복 입장 체크
	if (_monsters.find(mid) != _monsters.end())
		return;

	// 방 몬스터 목록에 추가하고 방 포인터 세팅
	_monsters.insert({ mid, monster });
	monster->SetRoom(shared_from_this());

	// Grid 시스템에 몬스터 등록. 어느 Zone에 있는지 기록함
	int32 zoneIndex = _grid.GetZoneIndex(*monster->GetPosInfo());
	monster->SetZoneIndex(zoneIndex);
	_grid.GetZone(zoneIndex).monsters.insert(monster);

	if (ExperimentUtils::IsHotRoomRoomWideBaseline())
	{
		Protocol::S_SPAWN spawnPkt;
		auto* mInfo = spawnPkt.add_monsters();
		*mInfo = *monster->GetMonsterInfo();
		SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(spawnPkt);

		auto& viewers = monster->Viewers_ActorOnly();
		viewers.clear();

		for (auto& item : _players)
		{
			PlayerRef p = item.second;
			if (!p)
				continue;

			p->VisibleMonsters_ActorOnly().insert(mid);
			viewers.insert(p->GetPlayerId());
		}

		const int32 recipients = Broadcast(sb);
		GameMetrics::OnBroadcastRecipients(
			GameMetrics::HotRoomBroadcastKind::Spawn,
			GameMetrics::HotRoomBroadcastMode::Room,
			static_cast<std::size_t>(recipients));
		return;
	}

	// AOI 처리: 주변 Zone들을 긁어와서 시야 범위 내 플레이어 찾기
	Vector<Zone*> zones;
	_grid.GetNearbyZones(zoneIndex, EffectiveAoiRadiusCells(), zones);

	Protocol::S_SPAWN spawnPkt;
	auto* mInfo = spawnPkt.add_monsters();
	*mInfo = *monster->GetMonsterInfo();
	SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(spawnPkt);

	// 이 몬스터를 보고 있는 플레이어 목록 초기화
	auto& viewers = monster->Viewers_ActorOnly();
	viewers.clear();
	std::size_t spawnRecipients = 0;

	const auto& mp = *monster->GetPosInfo();
	const uint32 mConn = GetConnectivityId_ActorOnly(mp); // 벽 너머에 있는 애들은 제외하려고 ID 확인

	for (Zone* z : zones)
	{
		for (const PlayerRef& p : z->players)
		{
			if (!p) continue;

			// 거리 체크해서 시야 밖이면 패스
			const auto& pp = *p->GetPosInfo();
			if (!PassDistance2D(mp, pp, _interestRadius))
				continue;

			// 벽으로 막혀있는지 확인 (Connectivity)
			const uint32 pConn = GetConnectivityId_ActorOnly(pp);
			if (pConn != mConn)
				continue;

			// 플레이어의 시야 목록에도 이 몬스터 추가 (서버 동기화)
			if (p->VisibleMonsters_ActorOnly().insert(mid).second)
			{
				viewers.insert(p->GetPlayerId());
				SendToPlayer(p->GetPlayerId(), sb); // 클라한테 스폰 패킷 전송
				++spawnRecipients;
			}
			else
			{
				// 이미 보고 있었으면 목록 관리만 함
				viewers.insert(p->GetPlayerId());
			}
		}
	}

	GameMetrics::OnBroadcastRecipients(
		GameMetrics::HotRoomBroadcastKind::Spawn,
		GameMetrics::HotRoomBroadcastMode::Aoi,
		spawnRecipients);
}

void GameRoom::LeaveMonster(uint64 objectId)
{
	auto it = _monsters.find(objectId);
	if (it == _monsters.end()) return;

	MonsterRef m = it->second;
	if (!m) return;

	const int32 zoneIndex = m->GetZoneIndex();

	// 이 몬스터를 보고 있던 플레이어들에게만 디스폰 패킷 보냄
	{
		Protocol::S_DESPAWN pkt;
		pkt.add_objectids(objectId);
		SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);

		if (ExperimentUtils::IsHotRoomRoomWideBaseline())
		{
			auto& viewers = m->Viewers_ActorOnly();
			for (uint64 pid : viewers)
			{
				PlayerRef p = FindPlayer_ActorOnly(pid);
				if (p)
					p->VisibleMonsters_ActorOnly().erase(objectId);
			}

			const int32 recipients = Broadcast(sb);
			GameMetrics::OnBroadcastRecipients(
				GameMetrics::HotRoomBroadcastKind::Despawn,
				GameMetrics::HotRoomBroadcastMode::Room,
				static_cast<std::size_t>(recipients));
			viewers.clear();
		}
		else
		{

			auto& viewers = m->Viewers_ActorOnly();
			std::size_t despawnRecipients = 0;
			for (uint64 pid : viewers)
			{
				PlayerRef p = FindPlayer_ActorOnly(pid);
				if (p)
					p->VisibleMonsters_ActorOnly().erase(objectId); // 플레이어 시야 목록에서 삭제

				SendToPlayer(pid, sb);
				++despawnRecipients;
			}
			viewers.clear();
			GameMetrics::OnBroadcastRecipients(
				GameMetrics::HotRoomBroadcastKind::Despawn,
				GameMetrics::HotRoomBroadcastMode::Aoi,
				despawnRecipients);
		}
	}

	// Grid 시스템에서 몬스터 제거
	int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
	if (zoneIndex >= 0 && zoneIndex < totalZones)
		_grid.GetZone(zoneIndex).monsters.erase(m);

	_monsters.erase(objectId);
	m->SetRoom(nullptr);
}

void GameRoom::OnMonsterMoved(MonsterRef monster)
{
	if (!monster) return;

	const uint64 now = ::GetTickCount64();
	const uint64 mid = monster->GetObjectId();

	int32 oldZoneIndex = monster->GetZoneIndex();
	int32 newZoneIndex = _grid.GetZoneIndex(*monster->GetPosInfo());
	const bool zoneChanged = (newZoneIndex != oldZoneIndex);

	// Zone이 바뀌었으면 Grid 정보 갱신 (이건 즉시 해야 함)
	if (zoneChanged)
	{
		int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
		if (oldZoneIndex >= 0 && oldZoneIndex < totalZones)
			_grid.GetZone(oldZoneIndex).monsters.erase(monster);

		_grid.GetZone(newZoneIndex).monsters.insert(monster);
		monster->SetZoneIndex(newZoneIndex);
	}

	if (ExperimentUtils::IsHotRoomRoomWideBaseline())
	{
		Protocol::S_MOVE movePkt;
		movePkt.set_objectid(mid);
		*movePkt.mutable_posinfo() = *monster->GetPosInfo();
		SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(movePkt);

		const int32 recipients = Broadcast(sb);
		GameMetrics::OnBroadcastRecipients(
			GameMetrics::HotRoomBroadcastKind::Move,
			GameMetrics::HotRoomBroadcastMode::Room,
			static_cast<std::size_t>(recipients));
		return;
	}

	// 1단계: Cheap Update - 일단 보고 있던 사람들한테 이동 패킷만 쏨
	// 시야 목록 갱신은 비용이 비싸니까 매번 안 함
	{
		Protocol::S_MOVE movePkt;
		movePkt.set_objectid(mid);
		*movePkt.mutable_posinfo() = *monster->GetPosInfo();
		SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(movePkt);

		auto& viewers = monster->Viewers_ActorOnly();
		std::size_t moveRecipients = 0;
		for (uint64 pid : viewers)
		{
			SendToPlayer(pid, sb);
			++moveRecipients;
		}

		GameMetrics::OnBroadcastRecipients(
			GameMetrics::HotRoomBroadcastKind::Move,
			GameMetrics::HotRoomBroadcastMode::Aoi,
			moveRecipients);
	}

	// 2단계: Expensive Update 트리거 체크
	// 많이 움직였거나 시간이 좀 지났을 때만 시야 목록 새로 계산함 (Lazy Update)
	float lastX, lastZ;
	monster->GetLastAoiExpensivePos(lastX, lastZ);

	const float dx = monster->GetPosInfo()->x() - lastX;
	const float dz = monster->GetPosInfo()->z() - lastZ;
	const float moved = std::sqrt(dx * dx + dz * dz);

	const uint64 lastMs = monster->GetLastAoiExpensiveMs();

	const bool needExpensive =
		zoneChanged || // 존 바뀌면 무조건 해야 함
		(lastMs == 0) ||
		(now - lastMs >= _lazyUpdateTickMs) || // 시간 지남
		(moved >= _lazyUpdateDist); // 많이 움직임

	if (!needExpensive)
		return;

	// 3단계: Expensive Update 수행
	// 새로 보여야 할 플레이어(New)랑 이제 안 보여야 할 플레이어(Old) 계산해서 처리

	// 새 Viewers 목록 계산
	HashSet<uint64> newViewers;

	Vector<Zone*> zones;
	_grid.GetNearbyZones(monster->GetZoneIndex(), EffectiveAoiRadiusCells(), zones);

	const auto& mp = *monster->GetPosInfo();
	const uint32 mConn = GetConnectivityId_ActorOnly(mp);

	for (Zone* z : zones)
	{
		for (const PlayerRef& p : z->players)
		{
			if (!p) continue;

			const auto& pp = *p->GetPosInfo();
			if (!PassDistance2D(mp, pp, _interestRadius))
				continue;

			const uint32 pConn = GetConnectivityId_ActorOnly(pp);
			if (pConn != mConn)
				continue;

			newViewers.insert(p->GetPlayerId());
		}
	}

	auto& oldViewers = monster->Viewers_ActorOnly();

	// Despawn 처리: 예전엔 봤는데 이제 못 보는 애들 (Old - New)
	{
		Protocol::S_DESPAWN pkt;
		pkt.add_objectids(mid);
		SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);
		std::size_t despawnRecipients = 0;

		for (uint64 pid : oldViewers)
		{
			if (newViewers.find(pid) != newViewers.end())
				continue;

			PlayerRef p = FindPlayer_ActorOnly(pid);
			if (p) p->VisibleMonsters_ActorOnly().erase(mid);
			SendToPlayer(pid, sb);
			++despawnRecipients;
		}

		GameMetrics::OnBroadcastRecipients(
			GameMetrics::HotRoomBroadcastKind::Despawn,
			GameMetrics::HotRoomBroadcastMode::Aoi,
			despawnRecipients);
	}

	// Spawn 처리: 새로 보게 된 애들 (New - Old)
	{
		Protocol::S_SPAWN pkt;
		auto* mInfo = pkt.add_monsters();
		*mInfo = *monster->GetMonsterInfo();
		SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);
		std::size_t spawnRecipients = 0;

		for (uint64 pid : newViewers)
		{
			if (oldViewers.find(pid) != oldViewers.end())
				continue;

			PlayerRef p = FindPlayer_ActorOnly(pid);
			if (p) p->VisibleMonsters_ActorOnly().insert(mid);
			SendToPlayer(pid, sb);
			++spawnRecipients;
		}

		GameMetrics::OnBroadcastRecipients(
			GameMetrics::HotRoomBroadcastKind::Spawn,
			GameMetrics::HotRoomBroadcastMode::Aoi,
			spawnRecipients);
	}

	// 갱신된 목록으로 교체하고 타임스탬프 찍음
	oldViewers = std::move(newViewers);

	monster->SetLastAoiExpensiveMs(now);
	monster->SetLastAoiExpensivePos(monster->GetPosInfo()->x(), monster->GetPosInfo()->z());
}

// 범위 내 가장 가까운 플레이어 찾는 유틸 함수 (어그로 로직용)
PlayerRef GameRoom::FindNearestPlayer(Protocol::PositionInfo* pos, float range)
{
	if (!pos) return nullptr;

	// 전체 검색하면 느리니까 Grid 이용해서 주변 Zone만 뒤짐
	const int32 zoneIndex = _grid.GetZoneIndex(*pos);

	Vector<Zone*> zones;
	_grid.GetNearbyZones(zoneIndex, EffectiveAoiRadiusCells(), zones);

	// 벽 너머에 있는 플레이어는 타겟팅 안 되게 막음
	const uint32 myConn = GetConnectivityId_ActorOnly(*pos);

	PlayerRef target = nullptr;
	const float rangeSqr = range * range;
	float minDistSqr = rangeSqr;

	for (Zone* zone : zones)
	{
		for (const PlayerRef& player : zone->players)
		{
			if (!player) continue;
			if (player->GetStatInfo() && player->GetStatInfo()->hp() <= 0) continue; // 죽은 애는 무시

			const auto& pp = *player->GetPosInfo();

			// 거리 계산 (제곱으로 비교해서 sqrt 연산 줄임)
			const float dx = pp.x() - pos->x();
			const float dz = pp.z() - pos->z();
			const float distSqr = dx * dx + dz * dz;
			if (distSqr > rangeSqr)
				continue;

			if (GetConnectivityId_ActorOnly(pp) != myConn)
				continue;

			// 더 가까운 애 찾았으면 갱신
			if (distSqr < minDistSqr)
			{
				minDistSqr = distSqr;
				target = player;
			}
		}
	}

	return target;
}
