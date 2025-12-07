#include "pch.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "GameSessionManager.h"
#include "GameRoom.h" // Room 기능을 쓰려면 cpp에서 include

void PlayerSession::OnConnected()
{
	// [Manager] 전체 접속자 명단에 등록
	// (참조 카운트 증가)
	GameSessionManager::GSessionManager->Add(static_pointer_cast<PlayerSession>(shared_from_this()));
}

void PlayerSession::OnDisconnected()
{
	// [Manager] 전체 접속자 명단에서 제거
	// (참조 카운트 감소)
	GameSessionManager::GSessionManager->Remove(static_pointer_cast<PlayerSession>(shared_from_this()));

	// [Room] 만약 방에 있었다면 퇴장 처리 (비동기)
	// 방에 "나 나간다"고 Job을 던져줘야 다른 유저들에게 Despawn 패킷이 날아감
	if (shared_ptr<GameRoom> room = _room.lock())
	{
		// [Async Job] Room Lock 없이 안전하게 퇴장
		room->PushJob(&GameRoom::Leave, static_pointer_cast<PlayerSession>(shared_from_this()));

		// 더 이상 룸을 잡고 있을 필요 없음
		_room.reset();
	}
}

void PlayerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	// 패킷 핸들러에게 토스
	// (여기서 this를 넘겨주므로 핸들러가 세션 정보를 알 수 있음)
	PacketSessionRef session = GetPacketSessionRef();
	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void PlayerSession::OnSend(int32 len)
{
	// 전송 완료 후 추가 작업이 필요하면 작성
}