#include "pch.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "GameSessionManager.h"
#include "GameRoom.h" 
#include "Player.h" // [필수] Player의 멤버 함수(GetRoom)를 쓰려면 헤더가 있어야 함

void PlayerSession::OnConnected()
{
	// [Manager] 전체 접속자 명단에 등록
	GameSessionManager::GSessionManager->Add(static_pointer_cast<PlayerSession>(shared_from_this()));
}

void PlayerSession::OnDisconnected()
{
	// [Manager] 전체 접속자 명단에서 제거
	GameSessionManager::GSessionManager->Remove(static_pointer_cast<PlayerSession>(shared_from_this()));

	// [Room] 방 퇴장 처리
	// 예전엔 _room을 직접 봤지만, 이젠 _player한테 물어봐야 한다.
	if (_player)
	{
		// 1. 플레이어가 속한 방이 있는지 확인
		if (shared_ptr<GameRoom> room = _player->GetRoom())
		{
			// [Async Job] 방에 퇴장 요청
			// 주의: GameRoom::Leave 함수가 PlayerSession을 받는지 Player를 받는지에 따라 인자가 달라짐.
			// 기존 코드가 Session을 넘겼으니 일단 그대로 둠.
			room->PushJob(&GameRoom::Leave, static_pointer_cast<PlayerSession>(shared_from_this()));

			// 플레이어에게서 방 정보 삭제
			_player->SetRoom(nullptr);
		}

		// 2. [Critical] 순환 참조 해제 (Session <-> Player)
		// 이걸 안 하면 유저가 나가도 메모리에 Player와 Session이 영원히 남는다 (Memory Leak)
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
	// ClientPacketHandler에 있는 MakeSendBuffer를 사용한다.
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);

	Send(sendBuffer);
	std::cout << "[Server] Ping -> Client" << std::endl;
}