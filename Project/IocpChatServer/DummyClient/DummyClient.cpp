// DummyClient/Main/DummyClient.cpp  (새 메인 권장)
// 기존 include 유지 + 약간 추가
#include "pch.h"                 // [1] 항상 최상단
#include "ThreadManager.h"       // [2] 스레드 매니저
#include "Service.h" 
#include "Session.h"
#include "BufferReader.h"
#include "ServerPacketHandler.h" // [2] 핸들러 (세션 필요하므로 Session 다음)
#include "Protocol.pb.h"         // [3] 자동 생성된 프로토콜

// [4] 표준 라이브러리
#include <atomic>
#include <csignal>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>


// =====[글로벌: 기존 핸들러와의 호환 위해 일단 유지]=====
std::atomic<bool> g_isLoggedIn = false;   // 로그인 완료 여부 (핸들러가 true로 바꿔줌)
std::atomic<bool> g_isInRoom = false;   // 방 입장 여부    (핸들러가 true/false로 바꿔줌)
PacketSessionRef  g_session = nullptr; // 현재 세션 (패킷 보낼 때 사용)

// =====[간단 유틸]=====
static inline std::string Trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// 콘솔 안전 출력(멀티스레드)
std::mutex g_coutMtx;
#define TS_COUT(expr) do { std::lock_guard<std::mutex> _lk(g_coutMtx); std::cout << expr << std::endl; } while(0)

// =====[세션]=====
class ServerSession : public PacketSession
{
public:
    ~ServerSession() override { TS_COUT("~ServerSession"); }

    void OnConnected() override
    {
        g_session = GetPacketSessionRef();
        TS_COUT("[Connected] Successfully connected to the server. Use /login <name> first.");
    }


    void OnRecvPacket(BYTE* buffer, int32 len) override
    {
        auto session = GetPacketSessionRef();
        ServerPacketHandler::HandlePacket(session, buffer, len);
    }
    void OnSend(int32 len) override
    {
        TS_COUT("[Send] " << len << " bytes");
    }

    void OnDisconnected() override
    {
        g_isLoggedIn = false;
        g_isInRoom = false;
        g_session = nullptr;
        TS_COUT("[Disconnected] Connection to server lost");
    }
};

// =====[전역 종료 플래그]=====
std::atomic<bool> g_running = true;

void SigIntHandler(int) {
    g_running = false;
    TS_COUT("\nShutdown signal received: exiting...");
}

// =====[명령 처리]=====
void PrintHelp()
{
    TS_COUT(
        "\n=== Commands ===\n"
        "/help                          : Show this help\n"
        "/login <name>                  : Send login request\n"
        "/create <roomName>             : Create a room\n"
        "/join <roomId>                 : Join a room\n"
        "/leave                         : Leave the current room\n"
        "/say <message>                 : Send chat message (inside a room)\n"
        "/quit                          : Quit client\n"
    );
}

bool EnsureSessionReady()
{
    if (!g_session) {
        TS_COUT("Not connected to server yet.");
        return false;
    }
    return true;
}

void HandleCommandLine(const std::string& lineRaw)
{
    std::string line = Trim(lineRaw);
    if (line.empty()) return;

    if (line == "/help") { PrintHelp(); return; }
    if (line == "/quit") { g_running = false; return; }

    // 토큰화
    std::istringstream iss(line);
    std::string cmd; iss >> cmd;

    if (cmd == "/login") {
        std::string name; iss >> name;
        if (name.empty()) { TS_COUT("Usage: /login <name>"); return; }
        if (!EnsureSessionReady()) return;

        Protocol::C_LOGIN_REQ pkt;
        pkt.set_name(name);
        g_session->Send(ServerPacketHandler::MakeSendBuffer(pkt));
        TS_COUT("Login request -> " << name);
        return;
    }

    if (cmd == "/create") {
        if (!g_isLoggedIn) { TS_COUT("You must log in first."); return; }
        std::string roomName; std::getline(iss, roomName);
        roomName = Trim(roomName);
        if (roomName.empty()) { TS_COUT("Usage: /create <roomName>"); return; }
        if (!EnsureSessionReady()) return;

        Protocol::C_CREATE_ROOM_REQ pkt;
        pkt.set_roomname(roomName);
        pkt.set_type(Protocol::ROOM_GROUP);
        g_session->Send(ServerPacketHandler::MakeSendBuffer(pkt));
        TS_COUT("Create room request -> '" << roomName << "'");
        return;
    }

    if (cmd == "/join") {
        if (!g_isLoggedIn) { TS_COUT("You must log in first."); return; }
        int roomId;
        if (!(iss >> roomId)) { TS_COUT("Usage: /join <roomId(number)>"); return; }
        if (!EnsureSessionReady()) return;

        Protocol::C_JOIN_ROOM_REQ pkt;
        pkt.set_roomid(roomId);
        g_session->Send(ServerPacketHandler::MakeSendBuffer(pkt));
        TS_COUT("Join room request -> " << roomId);
        return;
    }

    if (cmd == "/leave") {
        if (!g_isLoggedIn) { TS_COUT("You must log in first."); return; }
        if (!g_isInRoom) { TS_COUT("You are already outside of a room."); return; }
        if (!EnsureSessionReady()) return;

        Protocol::C_LEAVE_ROOM_REQ pkt;
        g_session->Send(ServerPacketHandler::MakeSendBuffer(pkt));
        TS_COUT("Leave room request");
        return;
    }

    if (cmd == "/say") {
        if (!g_isLoggedIn) { TS_COUT("You must log in first."); return; }
        if (!g_isInRoom) { TS_COUT("Join a room first to use this."); return; }
        std::string msg; std::getline(iss, msg);
        msg = Trim(msg);
        if (msg.empty()) { TS_COUT("Usage: /say <message>"); return; }
        if (!EnsureSessionReady()) return;

        Protocol::C_ROOM_CHAT_REQ pkt;
        pkt.set_message(msg);
        g_session->Send(ServerPacketHandler::MakeSendBuffer(pkt));
        return;
    }

    TS_COUT("Unknown command. Type /help to see available commands.");
}

// =====[입력 스레드]=====
void InputThread()
{
    PrintHelp();
    std::string line;
    while (g_running) {
        {
            std::lock_guard<std::mutex> _lk(g_coutMtx);
            std::cout << "> " << std::flush;
        }
        if (!std::getline(std::cin, line)) {
            // EOF or error
            g_running = false;
            break;
        }
        HandleCommandLine(line);
    }
}

// =====[네트워크 디스패처 스레드]=====
void NetworkThread(ClientServiceRef service)
{
    while (g_running) {
        service->GetIocpCore()->Dispatch();
        // 과도한 busy-loop 방지 (IOCP는 보통 블로킹 대기지만 구현에 맞춰 소폭 양보)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main()
{
    std::signal(SIGINT, SigIntHandler);

    ServerPacketHandler::Init();

    // 서비스 생성/시작
    ClientServiceRef service = MakeShared<ClientService>(
        NetAddress(L"127.0.0.1", 7777),
        MakeShared<IocpCore>(),
        MakeShared<ServerSession>,
        1);

    ASSERT_CRASH(service->Start());

    // 스레드 런칭
    GThreadManager->Launch([service] { NetworkThread(service); });
    std::thread inputThread(InputThread);

    // 메인 스레드는 종료 대기
    inputThread.join();
    g_running = false;

    // 서비스/스레드 정리
    GThreadManager->Join(); // 네트워크 스레드 종료 대기
    return 0;
}
