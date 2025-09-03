#include "pch.h"
#include "ServerPacketHandler.h"


PacketHandlerFunc GPacketHandler[UINT16_MAX];
extern std::atomic<bool> g_isLoggedIn;
extern std::atomic<bool> g_isInRoom;
extern PacketSessionRef g_session;
// 직접 컨텐츠 작업자

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	return false;
}



// S_LOGIN_RES 패킷을 처리하는 핸들러 함수
bool Handle_S_LOGIN_RES(PacketSessionRef& session, Protocol::S_LOGIN_RES& pkt)
{
	if (pkt.status() == Protocol::CONNECT_OK)
	{
		// [핵심 수정] 로그인 성공 시 g_isLoggedIn 플래그를 true로 변경합니다.
		g_isLoggedIn = true;

		std::cout << "\n로그인 성공! Player ID: " << pkt.playerid() << std::endl;
		std::cout << "서버 메시지: " << pkt.reason() << std::endl;

		// [추가] 다음 입력을 받기 전, 혹시 남아있을 수 있는 입력 버퍼(특히 개행 문자)를 비웁니다.
		// 이렇게 하면 main 루프에서 메뉴 입력을 받을 때 발생할 수 있는 오류를 방지합니다.
		// std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	else
	{
		std::cout << "\n로그인 실패! 이유: " << pkt.reason() << std::endl;
		// 실패 시 다시 로그인할 수 있도록 유도하거나 프로그램을 종료할 수 있습니다.
	}

	return true;
}
bool Handle_S_RECONNECT_RES(PacketSessionRef& session, Protocol::S_RECONNECT_RES& pkt)
{
	return false;
}


bool Handle_S_PRIVATE_CHAT_NTF(PacketSessionRef& session, Protocol::S_PRIVATE_CHAT_NTF& pkt)
{
	return false;
}
bool Handle_S_JOIN_ROOM_NTF(PacketSessionRef& session, Protocol::S_JOIN_ROOM_NTF& pkt)
{
	std::cout << pkt.name() << " 님이 방에 들어왔습니다!" << std::endl;
	return true;
}


bool Handle_S_CREATE_ROOM_RES(PacketSessionRef& session, Protocol::S_CREATE_ROOM_RES& pkt)
{
	if (pkt.success())
	{
		const Protocol::RoomInfo& roomInfo = pkt.room();

		// [핵심] 방 생성과 동시에 입장된 상태로 처리
		g_isInRoom = true;

		std::cout << "\n==============================================" << std::endl;
		std::cout << "Room created successfully! Room ID: " << roomInfo.roomid()
			<< ", Room Name: " << roomInfo.roomname() << std::endl;
		std::cout << "채팅을 시작하세요! (나가려면 /exit 입력)" << std::endl;
		std::cout << "--- 현재 접속자 ---" << std::endl;

		for (const auto& member : roomInfo.members())
		{
			std::cout << "- " << member.name() << " (ID: " << member.playerid() << ")" << std::endl;
		}
		std::cout << "==============================================" << std::endl;
	}
	else
	{
		std::cout << "Failed to create room. Reason: " << pkt.reason() << std::endl;
	}
	return true;
}


bool Handle_S_JOIN_ROOM_RES(PacketSessionRef& session, Protocol::S_JOIN_ROOM_RES& pkt)
{
	if (pkt.success())
	{
		// [핵심 수정] 방 참가 성공 시 g_isInRoom 플래그를 true로 변경합니다.
		g_isInRoom = true;

		const Protocol::RoomInfo& roomInfo = pkt.room();
		std::cout << "\n==============================================" << std::endl;
		std::cout << "[" << roomInfo.roomname() << "] 방 (ID: " << roomInfo.roomid() << ")에 입장했습니다." << std::endl;
		std::cout << "채팅을 시작하세요! (나가려면 /exit 입력)" << std::endl;
		std::cout << "--- 현재 접속자 ---" << std::endl;

		for (const auto& member : roomInfo.members())
		{
			std::cout << "- " << member.name() << " (ID: " << member.playerid() << ")" << std::endl;
		}
		std::cout << "==============================================" << std::endl;
	}
	else
	{
		std::cout << "\n방 참가 실패! 이유: " << pkt.reason() << std::endl;
	}

	return true;
}

bool Handle_S_ROOM_CHAT_NTF(PacketSessionRef& session, Protocol::S_ROOM_CHAT_NTF& pkt)
{
	int64 roomId = pkt.roomid();
	const Protocol::ChatMessage& chatMsg = pkt.chat();

	uint64 messageId = chatMsg.messageid();
	int64 senderId = chatMsg.senderid();
	const std::string& message = chatMsg.message();
	int64 timestamp = chatMsg.timestamp();

	// [1] 디버깅 로그 출력 (임시)
	std::cout << "[Room " << roomId << "] "
		<< "Player(" << senderId << "): "
		<< message << " (msgId=" << messageId
		<< ", ts=" << timestamp << ")" << std::endl;
	/*
	// [2] 실제 클라 UI 쪽에 전달 (ChatManager 같은 곳으로 넘김)
	if (GChatManager != nullptr)
	{
		GChatManager->OnChatMessage(roomId, senderId, message, timestamp, messageId);
	}
	*/

	return true;
}

bool Handle_S_LEAVE_ROOM_ACK(PacketSessionRef& session, Protocol::S_LEAVE_ROOM_ACK& pkt)
{
	if (pkt.success())
	{
		g_isInRoom = false; // 방 나가기 성공 시 플래그 변경
		std::cout << "\n방에서 성공적으로 나갔습니다." << std::endl;
		std::cout << "메인 메뉴로 돌아갑니다." << std::endl;
	}
	else
	{
		std::cout << "\n방 나가기 실패! "  << std::endl;
	}
	return true;
}


bool Handle_S_FRIEND_ADD_RES(PacketSessionRef& session, Protocol::S_FRIEND_ADD_RES& pkt)
{
	return false;
}

bool Handle_S_FRIEND_LIST_RES(PacketSessionRef& session, Protocol::S_FRIEND_LIST_RES& pkt)
{
	return false;
}

bool Handle_S_PRESENCE_NTF(PacketSessionRef& session, Protocol::S_PRESENCE_NTF& pkt)
{
	return false;
}

bool Handle_S_RATE_LIMIT_NTF(PacketSessionRef& session, Protocol::S_RATE_LIMIT_NTF& pkt)
{
	return false;
}

bool Handle_S_ADMIN_COMMAND_RES(PacketSessionRef& session, Protocol::S_ADMIN_COMMAND_RES& pkt)
{
	return false;
}

