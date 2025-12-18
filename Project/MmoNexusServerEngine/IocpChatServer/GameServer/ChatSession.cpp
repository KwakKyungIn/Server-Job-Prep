#include "pch.h"
#include "ChatSession.h"
#include "S2SPacketHandler.h" // [Chat Response]

void ChatSession::OnConnected()
{
    std::cout << "✅ [GameServer] Connected To ChatServer!" << std::endl;

    // Step1 테스트: 멤버십 sync + 파티 채팅 1회 송신
    {
        Protocol::S2S_REQ_PARTY_SYNC sync;
        sync.set_partyid(1);
        sync.set_leaderid(100);
        sync.add_memberids(100);
        sync.add_memberids(200);
        sync.set_version(1);

        auto sendBuffer = S2SPacketHandler::MakeSendBuffer(sync);
        Send(sendBuffer);
    }

    {
        Protocol::S2S_REQ_PARTY_CHAT chat;
        chat.set_partyid(1);
        chat.set_senderid(100);
        chat.set_sendername("Leader_100");
        chat.set_message("hello party");
        chat.set_version(1);

        auto sendBuffer = S2SPacketHandler::MakeSendBuffer(chat);
        Send(sendBuffer);
    }
}


void ChatSession::OnDisconnected()
{
	std::cout << "❌ [GameServer] Disconnected From ChatServer" << std::endl;
}

void ChatSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	// ChatServer가 보낸 응답 처리 (방송 성공 여부 등)
	// S2S 핸들러를 공유해서 쓴다.
	S2SPacketHandler::HandlePacket(session, buffer, len);
}

void ChatSession::OnSend(int32 len)
{
}

void ChatSession::Ping()
{
	std::cout << "GAME -> CHAT" << std::endl;
	Protocol::S2S_REQ_HEART_BEAT pkt;
	auto sendBuffer = S2SPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);
}