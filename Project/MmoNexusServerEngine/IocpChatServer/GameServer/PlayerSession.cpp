#include "pch.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "GameSessionManager.h"
#include "GameRoom.h"
#include "Player.h"

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

    Post([=](PlayerSessionRef ps)
        {
            ps->CancelMapChange();

            if (ps->_player)
            {
                // ✅ player를 먼저 로컬로 잡는다 (shared_ptr 복사)
                PlayerRef player = ps->_player;

                // 룸 나가기는 룸 Actor에게 요청
                if (auto room = ps->GetRoom())   // ✅ 이제 session cache
                {
                    room->PushJob(&GameRoom::Leave, ps, player);
                }
                ps->SetCurrentRoom(nullptr);     // ✅ 더 이상 라우팅 안 타게 즉시 끊기

                // 순환 참조 해제는 세션이 처리
                player->SetSession(nullptr);
                ps->_player = nullptr;
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


bool PlayerSession::TryBeginMapChange(uint64 token, int32 targetMapId, const Protocol::PositionInfo& spawn)
{
    std::lock_guard<std::mutex> lock(_mapChangeLock);

    if (_mapChangeState.load(std::memory_order_relaxed) != MAP_CHANGE_NONE)
        return false;

    _mapChangeToken = token;
    _pendingTargetMapId = targetMapId;
    _pendingSpawn.CopyFrom(spawn);

    _mapChangeState.store(MAP_CHANGE_WAITING_ACK, std::memory_order_release);
    return true;
}

void PlayerSession::ResetMapChangeState_Locked()
{
    // ⚠️ 이 함수는 _mapChangeLock을 이미 잡은 상태에서만 호출해야 한다.
    _mapChangeToken = 0;
    _pendingTargetMapId = 0;
    _pendingSpawn.Clear();

    _mapChangeState.store(MAP_CHANGE_NONE, std::memory_order_release);
}

bool PlayerSession::TryConsumeMapChangeAck(uint64 token, int32& outTargetMapId, Protocol::PositionInfo& outSpawn)
{
    std::lock_guard<std::mutex> lock(_mapChangeLock);

    if (_mapChangeState.load(std::memory_order_relaxed) != MAP_CHANGE_WAITING_ACK)
        return false;

    if (_mapChangeToken != token)
        return false;

    outTargetMapId = _pendingTargetMapId;
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
