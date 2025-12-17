#include "pch.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "GameSessionManager.h"
#include "GameRoom.h"
#include "Player.h"

void PlayerSession::OnConnected()
{
	// [Manager] 전체 접속자 명단에 등록
	GameSessionManager::GSessionManager->Add(static_pointer_cast<PlayerSession>(shared_from_this()));
}

void PlayerSession::OnDisconnected()
{
	// 맵 이동 중이든 아니든, 끊기면 상태 리셋(토큰/락 해제)
	CancelMapChange();

	// [Manager] 전체 접속자 명단에서 제거
	GameSessionManager::GSessionManager->Remove(static_pointer_cast<PlayerSession>(shared_from_this()));

	// [Room] 방 퇴장 처리
	if (_player)
	{
		if (shared_ptr<GameRoom> room = _player->GetRoom())
		{
			room->PushJob(&GameRoom::Leave, static_pointer_cast<PlayerSession>(shared_from_this()));
			_player->SetRoom(nullptr);
		}

		// [Critical] 순환 참조 해제
		_player->SetSession(nullptr);
		_player = nullptr;
	}
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
