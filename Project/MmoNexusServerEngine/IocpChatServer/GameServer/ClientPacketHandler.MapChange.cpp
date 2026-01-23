#include "pch.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "GameRoom.h" 
#include "DataManager.h"
#include "ClientPacketHandler.MapChangeUtil.h"
#include "Player.h"

// 클라이언트가 포털을 타거나 텔레포트를 시도할 때 호출되는 핸들러
// 단순히 맵 ID만 확인하는 게 아니라 세션 상태를 변경하고 토큰을 발급하는 인증 과정을 거친다
bool ClientPacketHandler::Handle_C_MAP_CHANGE_REQ(PacketSessionRef& session, Protocol::C_MAP_CHANGE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	// 이미 맵 이동 중인데 또 요청이 오면 무시한다 (중복 패킷 방지)
	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	const int32 targetMapId = pkt.targetmapid();
	const int64 targetInstanceId = 0; // 일반 월드 맵 이동은 인스턴스 ID가 항상 0이다

	DataManager* dm = DataManager::Instance();

	// 데이터 매니저를 통해 이동하려는 맵이 서버에 실제로 존재하는지 검증
	// 유효하지 않은 맵으로 이동하면 클라이언트가 크래시 날 수 있으니 여기서 막는다
	if (!dm || !dm->IsValidMapId(targetMapId) || !dm->IsWorldMapId(targetMapId))
	{
		std::cout << " [MapChange] rejected. targetMapId=" << targetMapId << std::endl;
		return true;
	}

	const MapConfig* cfg = dm->GetMapConfig(targetMapId);
	if (!cfg)
		return true;

	// 해당 맵의 스폰 지점 좌표를 미리 가져온다
	Protocol::PositionInfo spawn;
	spawn.set_x(cfg->spawnX);
	spawn.set_y(cfg->spawnY);
	spawn.set_z(cfg->spawnZ);

	// 검증이 끝났으니 세션 액터에게 작업을 넘긴다
	// 세션 액터 내부에서 상태를 변경해야 동기화 문제가 발생하지 않음
	ps->Post([playerId, targetMapId, targetInstanceId, spawn](PlayerSessionRef self) mutable
		{
			if (self->IsMapChanging())
				return;

			// 이동할 채널 ID를 결정한다
			// 현재 게임 룸에 있다면 같은 채널을 유지하려고 시도함
			int32 targetChannelId = 1;
			if (auto room = self->GetCurrentRoom_ActorOnly())
			{
				if (room->GetKind() == RoomKind::Game)
				{
					auto gr = std::static_pointer_cast<GameRoom>(room);
					targetChannelId = gr->GetChannelId();
				}
			}

			// 위조된 이동 요청을 막기 위해 서버에서 고유 토큰을 생성한다
			const uint64 token = MapChangeUtil::MakeMapChangeToken(playerId, self->GetSessionId());

			// 세션 상태를 MapChanging으로 변경하여 다른 요청을 잠근다
			if (!self->TryBeginMapChange(token, targetChannelId, targetMapId, targetInstanceId, spawn))
				return;

			// 클라이언트에게 이동 시작 패킷을 보낸다
			// 클라이언트는 이 패킷을 받고 씬 로딩을 시작한 뒤 ACK를 보내야 한다
			Protocol::S_MAP_CHANGE_BEGIN beginPkt;
			beginPkt.set_token(token);
			beginPkt.set_targetmapid(targetMapId);
			beginPkt.mutable_spawn()->CopyFrom(spawn);
			beginPkt.set_instanceid(targetInstanceId);

			self->Send(MakeSendBuffer(beginPkt));
		});

	return true;
}

// 클라이언트가 로딩 준비가 끝났다고 신호를 보내면 호출되는 최종 처리 핸들러
// 여기서 실제 플레이어 객체의 메모리 이동(방 나가기 -> 방 들어가기)이 이루어진다
bool ClientPacketHandler::Handle_C_MAP_CHANGE_ACK(PacketSessionRef& session, Protocol::C_MAP_CHANGE_ACK& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	const uint64 token = pkt.token();

	// 세션 액터 컨텍스트에서 토큰 검증을 먼저 수행한다
	ps->Post([token](PlayerSessionRef self)
		{
			int32 targetChannelId = 0;
			int32 targetMapId = 0;
			int64 targetInstanceId = 0;
			Protocol::PositionInfo spawn;

			// 저장해둔 토큰과 클라이언트가 보낸 토큰이 일치하는지 확인하고
			// 이동할 목적지 정보를 꺼내온다
			if (!self->TryConsumeMapChangeAck(token, targetChannelId, targetMapId, targetInstanceId, spawn))
				return;

			const uint64 playerId = self->GetPlayerId_AnyThread();
			if (playerId == 0)
			{
				self->CancelMapChange();
				return;
			}

			auto room = self->GetCurrentRoom_ActorOnly();
			if (!room || room->GetKind() != RoomKind::Game)
			{
				self->CancelMapChange();
				return;
			}

			// 현재 플레이어가 있는 게임 룸 액터에게 작업을 위임한다
			// 플레이어 리스트 수정은 반드시 해당 룸의 스레드 안에서 해야 안전하다
			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, targetChannelId, targetMapId, targetInstanceId, spawn]() mutable
				{
					// GameRoom 클래스의 맵 이동 처리 함수 호출
					// 여기서 LeaveRoom -> EnterRoom(새 방) 로직이 순차적으로 실행됨
					gr->TransferMapChangeById(self, playerId, targetChannelId, targetMapId, targetInstanceId, spawn);
				});
		});

	return true;
}

// 맵 이동과 로직은 거의 같지만 맵 ID는 그대로 두고 채널만 바꾸는 핸들러
// 현재 위치를 그대로 유지한 채 채널만 이동해야 하므로 좌표 스냅샷이 필요하다
bool ClientPacketHandler::Handle_C_CHANNEL_CHANGE_REQ(PacketSessionRef& session, Protocol::C_CHANNEL_CHANGE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	const int32 targetChannelId = pkt.targetchannelid();
	if (targetChannelId <= 0)
		return true;

	ps->Post([playerId, targetChannelId](PlayerSessionRef self) mutable
		{
			if (self->IsMapChanging())
				return;

			auto room = self->GetCurrentRoom_ActorOnly();
			if (!room || room->GetKind() != RoomKind::Game)
				return;

			auto gr = std::static_pointer_cast<GameRoom>(room);

			// 이미 그 채널에 있으면 굳이 이동할 필요 없음
			const int32 curChannelId = gr->GetChannelId();
			if (targetChannelId == curChannelId)
				return;

			// 던전 인스턴스에서는 채널 이동을 금지한다 (정책상 막음)
			if (gr->IsInstanceRoom())
				return;

			// 현재 플레이어의 좌표를 정확히 알기 위해 룸 액터로 들어간다
			// 이동 후에도 같은 자리에 서 있어야 하기 때문
			gr->Push([gr, self, playerId, targetChannelId]() mutable
				{
					PlayerRef p = gr->FindPlayer_ActorOnly(playerId);
					if (!p) return;

					const int32 targetMapId = p->GetMapId();
					const int64 targetInstanceId = 0;

					Protocol::PositionInfo spawn;
					if (auto pos = p->GetPosInfo())
						spawn.CopyFrom(*pos);

					// 좌표 정보를 획득했으니 다시 세션 액터로 돌아와서 이동 프로세스 시작
					// 핑퐁 구조지만 데이터 안전성을 위해 필수적인 과정임
					self->Post([playerId, targetChannelId, targetMapId, targetInstanceId, spawn](PlayerSessionRef s) mutable
						{
							if (s->IsMapChanging())
								return;

							const uint64 token = MapChangeUtil::MakeMapChangeToken(playerId, s->GetSessionId());
							if (!s->TryBeginMapChange(token, targetChannelId, targetMapId, targetInstanceId, spawn))
								return;

							Protocol::S_MAP_CHANGE_BEGIN beginPkt;
							beginPkt.set_token(token);
							beginPkt.set_targetmapid(targetMapId);
							beginPkt.mutable_spawn()->CopyFrom(spawn);
							beginPkt.set_instanceid(targetInstanceId);
							beginPkt.set_targetchannelid(targetChannelId);

							s->Send(ClientPacketHandler::MakeSendBuffer(beginPkt));
						});
				});
		});

	return true;
}