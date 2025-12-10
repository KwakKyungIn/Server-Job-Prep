#include "pch.h"
#include "S2SPacketHandler.h"
#include "ClientPacketHandler.h"
#include "GameServerSession.h"
#include "RedisManager.h"   // [Core]
#include "CoreGlobal.h"
#include "LoginSessionManager.h"

PacketHandlerFunc S2SPacketHandler::GPacketHandler[UINT16_MAX];

bool S2SPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [RES] DB -> LoginServer : "로그인 결과 (ID: 100)"
bool S2SPacketHandler::Handle_S2S_RES_LOGIN(PacketSessionRef& session, Protocol::S2S_RES_LOGIN& pkt)
{
	std::cout << "🔍 [DEBUG] Handle_S2S_RES_LOGIN Called. Success: " << pkt.success() << std::endl;
	// 1. 요청했던 클라이언트 세션 찾기
	// [수정] LoginSessionManager를 사용해서 찾는다.
	// 반환 타입이 이미 shared_ptr<ClientSession>이므로 캐스팅 불필요!
	auto clientSession = LoginSessionManager::GSessionManager->Find(pkt.playersessionid());

	if (clientSession == nullptr)
	{
		std::cout << "💀 [DEBUG] ClientSession Not Found! ID: " << pkt.playersessionid() << std::endl;
		// 유저가 로그인 요청 후 응답 오기 전에 끊어짐
		return true;
	}

	Protocol::S_LOGIN resPkt;

	if (pkt.success())
	{
		// ==========================================================
		// [TOKEN GENERATION]
		// ==========================================================
		uint64 rawId = pkt.playerid();
		uint64 timestamp = ::GetTickCount64();
		int32 randomVal = rand();

		// "Token_{PlayerID}_{Timestamp}_{Random}"
		std::string token = "Token_" + std::to_string(rawId) + "_" + std::to_string(timestamp) + "_" + std::to_string(randomVal);

		// ==========================================================
		// [REDIS CACHING] 
		// ==========================================================
		if (GRedisManager->Set(token, std::to_string(rawId), 300)) // 5분 TTL
		{
			printf("🔑 [Redis] Token Saved: %s -> ID: %llu\n", token.c_str(), rawId);
		}

		resPkt.set_success(true);
		resPkt.set_token(token);

		// 게임 서버 정보
		auto* server = resPkt.add_serverlist();
		server->set_name("GIGACHAD Server 1");
		server->set_ip("127.0.0.1");
		server->set_port(7777);
		server->set_congestion(0);

		printf("✅ [Login] Success! User: %llu\n", rawId);
	}
	else
	{
		resPkt.set_success(false);
		printf("❌ [Login] Failed (Invalid ID/PW)\n");
	}

	// 3. 클라에게 전송
	clientSession->Send(ClientPacketHandler::MakeSendBuffer(resPkt));
	return true;
}

// 나머지 미사용 핸들러
bool S2SPacketHandler::Handle_S2S_RES_LOAD_PLAYER_DATA(PacketSessionRef& session, Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt) { return false; }
bool S2SPacketHandler::Handle_S2S_RES_ITEMS_LOAD(PacketSessionRef& session, Protocol::S2S_RES_ITEMS_LOAD& pkt) { return false; }
bool S2SPacketHandler::Handle_S2S_RES_LOAD_GAME_DATA(PacketSessionRef& session, Protocol::S2S_RES_LOAD_GAME_DATA& pkt) { return false; }
bool S2SPacketHandler::Handle_S2S_RES_BROADCAST_CHAT(PacketSessionRef& session, Protocol::S2S_RES_BROADCAST_CHAT& pkt) { return true; }
bool S2SPacketHandler::Handle_S2S_RES_HEART_BEAT(PacketSessionRef& session, Protocol::S2S_RES_HEART_BEAT& pkt) { return true; }