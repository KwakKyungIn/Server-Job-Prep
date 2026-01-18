#include "pch.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "GameSessionManager.h"
#include "GameRoom.h"
#include "Player.h"
#include "InstanceActor.h"
#include "RoomManager.h"
#include "AutoCommitService.h"
#include "PersistenceService.h"

void PlayerSession::OnConnected()
{
	ASSERT_CRASH(GameSessionManager::GSessionManager != nullptr);


	// [Manager] 전체 접속자 명단에 등록
	GameSessionManager::GSessionManager->Add(static_pointer_cast<PlayerSession>(shared_from_this()));
}

void PlayerSession::OnDisconnected()
{
    auto self = static_pointer_cast<PlayerSession>(shared_from_this());

    GameSessionManager::GSessionManager->Remove(self);

    //  모든 정리는 Session Actor thread에서
    Post([=](PlayerSessionRef ps)
        {
            ps->CancelMapChange();

            //  Session은 PlayerRef 금지. ID만 쓴다.
            const uint64 playerId = ps->GetPlayerId_AnyThread();

            //  라우팅 즉시 차단 (제일 먼저)
            RoomActorRef room = ps->GetCurrentRoom_ActorOnly();
            ps->SetCurrentRoom(nullptr);

            //  바인딩 정리
            if (playerId != 0)
            {
                //  [A] 안전빵: disconnect면 저장 트리거
            // (최적화는 나중에: invDirty 실제로 있을 때만 찍도록 바꿔도 됨)
                Persistence::PersistenceService::I().MarkDirty_PlayerCore(playerId);
                Persistence::PersistenceService::I().MarkDirty_Inventory(playerId);
                Persistence::AutoCommitService::I().RequestFlushNow(playerId);

                GameSessionManager::GSessionManager->UnbindPlayerId(playerId);
                ps->ClearPlayerId_ActorOnly();
            }

            //  룸에서 플레이어 제거(룸 스레드에서만)
            if (playerId != 0 && room && room->GetKind() == RoomKind::Game)
            {
                auto gr = std::dynamic_pointer_cast<GameRoom>(room);
                if (gr)
                {
                    // LeaveById는 아래 2)에서 추가해라
                    gr->PushJob(&GameRoom::LeaveById, ps, playerId);
                }
            }

            //  오프라인 강제 복귀 정책(인스턴스 멤버십 제거)
            if (playerId != 0)
            {
                InstanceActor::Instance().Push([playerId]()
                    {
                        InstanceManagerCore::InstanceInfo closed;
                        if (InstanceActor::Instance().Core().OnMemberOffline(playerId, closed))
                        {
                            if (closed.instanceId != 0 && GRoomManager)
                            {
                                auto r = GRoomManager->FindRoom(closed.channelId, closed.mapId, closed.instanceId);
                                if (r) r->MarkClosing(true);
                            }
                        }
                    });
            }
        });
}

void PlayerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void PlayerSession::OnSend(int32 len)
{
	// 전송 완료 후 추가 작업
}

void PlayerSession::Ping()
{
	Protocol::S_HEART_BEAT_RES pkt;
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);
	std::cout << "[Server] Ping -> Client" << std::endl;
}


bool PlayerSession::TryBeginMapChange(uint64 token, int32 targetChannelId, int32 targetMapId, int64 targetInstanceId, const Protocol::PositionInfo& spawn)
{
    std::lock_guard<std::mutex> lock(_mapChangeLock);

    if (_mapChangeState.load(std::memory_order_relaxed) != MAP_CHANGE_NONE)
        return false;

    _mapChangeToken = token;
    _pendingTargetChannelId = targetChannelId;      // 추가
    _pendingTargetMapId = targetMapId;
    _pendingTargetInstanceId = targetInstanceId;
    _pendingSpawn.CopyFrom(spawn);

    _mapChangeState.store(MAP_CHANGE_WAITING_ACK, std::memory_order_release);
    return true;
}

void PlayerSession::ResetMapChangeState_Locked()
{
    _mapChangeToken = 0;
    _pendingTargetChannelId = 0;                  
    _pendingTargetMapId = 0;
    _pendingTargetInstanceId = 0;
    _pendingSpawn.Clear();

    _mapChangeState.store(MAP_CHANGE_NONE, std::memory_order_release);
}

bool PlayerSession::TryConsumeMapChangeAck(uint64 token, int32& outTargetChannelId, int32& outTargetMapId, int64& outTargetInstanceId, Protocol::PositionInfo& outSpawn)
{
    std::lock_guard<std::mutex> lock(_mapChangeLock);

    if (_mapChangeState.load(std::memory_order_relaxed) != MAP_CHANGE_WAITING_ACK)
        return false;
    if (_mapChangeToken != token)
        return false;

    outTargetChannelId = _pendingTargetChannelId; 
    outTargetMapId = _pendingTargetMapId;
    outTargetInstanceId = _pendingTargetInstanceId;
    outSpawn.CopyFrom(_pendingSpawn);

    _mapChangeState.store(MAP_CHANGE_SWITCHING, std::memory_order_release);
    return true;
}

void PlayerSession::EndMapChange()
{
    std::lock_guard<std::mutex> lock(_mapChangeLock);
    ResetMapChangeState_Locked();
}

void PlayerSession::CancelMapChange()
{
    std::lock_guard<std::mutex> lock(_mapChangeLock);
    ResetMapChangeState_Locked();
}

uint64 PlayerSession::GetMapChangeToken() const
{
    std::lock_guard<std::mutex> lock(_mapChangeLock);
    return _mapChangeToken;
}
