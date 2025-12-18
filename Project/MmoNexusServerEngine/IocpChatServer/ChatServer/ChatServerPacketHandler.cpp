#include "pch.h"
#include "ChatServerPacketHandler.h"
#include "ChatServerSessionManager.h"
#include "PartyManager.h"

PacketHandlerFunc ChatServerPacketHandler::GPacketHandler[UINT16_MAX];

bool ChatServerPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
    return false;
}

// [Game -> Chat] 로그인 요청? 채팅서버는 로그인 처리 안 함.
// 하지만 상대 서버가 응답 대기하다 멈추는 걸 막기 위해 "실패 응답"은 돌려준다.
bool ChatServerPacketHandler::Handle_S2S_REQ_LOGIN(PacketSessionRef& session, Protocol::S2S_REQ_LOGIN& pkt)
{
    Protocol::S2S_RES_LOGIN resPkt;
    resPkt.set_success(false);

    auto sendBuffer = ChatServerPacketHandler::MakeSendBuffer(resPkt);
    session->Send(sendBuffer);
    return true;
}

// [Game -> Chat] 채팅 방송 요청 (핵심)
bool ChatServerPacketHandler::Handle_S2S_REQ_BROADCAST_CHAT(PacketSessionRef& session, Protocol::S2S_REQ_BROADCAST_CHAT& pkt)
{
    std::cout << "📢 [ChatServer] Broadcast: " << pkt.message() << " from " << pkt.name() << std::endl;

    Protocol::S2S_RES_BROADCAST_CHAT resPkt;
    resPkt.set_success(true);

    auto sendBuffer = ChatServerPacketHandler::MakeSendBuffer(resPkt);
    session->Send(sendBuffer);

    return true;
}

// [Game -> Chat] (원래 DB 쪽 기능) - ChatServer는 처리 안 함.
// 그래도 상대가 대기하지 않게 실패 응답은 반환.
bool ChatServerPacketHandler::Handle_S2S_REQ_LOAD_PLAYER_DATA(PacketSessionRef& session, Protocol::S2S_REQ_LOAD_PLAYER_DATA& pkt)
{
    Protocol::S2S_RES_LOAD_PLAYER_DATA resPkt;
    resPkt.set_success(false);
    resPkt.set_gamesessionid(pkt.gamesessionid()); // 있으면 넣어주면 디버깅 편함

    auto sendBuffer = ChatServerPacketHandler::MakeSendBuffer(resPkt);
    session->Send(sendBuffer);
    return true;
}

bool ChatServerPacketHandler::Handle_S2S_REQ_ITEMS_LOAD(PacketSessionRef& session, Protocol::S2S_REQ_ITEMS_LOAD& pkt)
{
    Protocol::S2S_RES_ITEMS_LOAD resPkt;
    resPkt.set_success(false);
    resPkt.set_gamesessionid(pkt.gamesessionid());

    auto sendBuffer = ChatServerPacketHandler::MakeSendBuffer(resPkt);
    session->Send(sendBuffer);
    return true;
}

bool ChatServerPacketHandler::Handle_S2S_REQ_LOAD_GAME_DATA(PacketSessionRef& session, Protocol::S2S_REQ_LOAD_GAME_DATA& pkt)
{
    Protocol::S2S_RES_LOAD_GAME_DATA resPkt;
    resPkt.set_success(false);

    auto sendBuffer = ChatServerPacketHandler::MakeSendBuffer(resPkt);
    session->Send(sendBuffer);
    return true;
}

bool ChatServerPacketHandler::Handle_S2S_REQ_HEART_BEAT(PacketSessionRef& session, Protocol::S2S_REQ_HEART_BEAT& pkt)
{
    Protocol::S2S_RES_HEART_BEAT resPkt;
    auto sendBuffer = ChatServerPacketHandler::MakeSendBuffer(resPkt);
    session->Send(sendBuffer);
    return true;
}

bool ChatServerPacketHandler::Handle_S2S_REQ_PARTY_SYNC(PacketSessionRef& session, Protocol::S2S_REQ_PARTY_SYNC& pkt)
{
    const uint64 partyId = pkt.partyid();
    const uint64 leaderId = pkt.leaderid();
    const uint32 version = pkt.version();

    std::vector<uint64> members;
    members.reserve(pkt.memberids_size());
    for (int i = 0; i < pkt.memberids_size(); ++i)
        members.push_back(pkt.memberids(i));

    PartyManager::Instance().Upsert(partyId, leaderId, version, members);

    std::cout << "[ChatServer] PARTY_SYNC partyId=" << partyId
        << " leaderId=" << leaderId
        << " members=" << (int)members.size()
        << " ver=" << version << std::endl;

    Protocol::S2S_RES_PARTY_SYNC res;
    res.set_success(true);
    res.set_partyid(partyId);
    res.set_version(version);

    // 여기서는 "요청한 GameServer"에게만 ACK 보내는 게 안전함
    session->Send(ChatServerPacketHandler::MakeSendBuffer(res));
    return true;
}

// --------------------
// STEP2 핵심: PARTY_CHAT
// GameServer -> ChatServer (중계 요청)
// ChatServer -> GameServer들 (브로드캐스트 echo)
// --------------------
bool ChatServerPacketHandler::Handle_S2S_REQ_PARTY_CHAT(PacketSessionRef& session, Protocol::S2S_REQ_PARTY_CHAT& pkt)
{
    const uint64 partyId = pkt.partyid();
    const uint64 senderId = pkt.senderid();
    const uint32 version = pkt.version();

    const std::string& senderName = pkt.sendername();
    const std::string& message = pkt.message();

    bool ok = PartyManager::Instance().IsMember(partyId, senderId);

    std::cout << "[ChatServer] PARTY_CHAT partyId=" << partyId
        << " senderId=" << senderId
        << " name=" << senderName
        << " msg=" << message
        << " ver=" << version << std::endl;

    Protocol::S2S_RES_PARTY_CHAT res;
    res.set_success(ok);
    res.set_partyid(partyId);
    res.set_senderid(senderId);
    res.set_sendername(senderName);
    res.set_message(message);
    res.set_version(version);

    auto sendBuf = ChatServerPacketHandler::MakeSendBuffer(res);

    // Step2: 단일 요청 세션에만 응답(ACK) 보장
    session->Send(sendBuf);

    // TODO(Step3): 멤버들에게 뿌리기 + 여러 GameServer 브로드캐스트는 "버퍼 복제"로 처리
    return true;
}
