// DummyClient/Main/DummyClient.cpp  — minimal fixed-profile version
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "ServerPacketHandler.h"
#include "Protocol.pb.h"
#include "ServerSession.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

#include <random>
using namespace std::chrono;

// ===== 고정 프로파일 =====
static constexpr int   kClients = 500;
static constexpr int   kRooms = 10;
static constexpr double kRps = 1.0;   // per client

// === 시뮬레이터 ===
class ClientSimulator
{
public:
    void Start()
    {
        _core = MakeShared<IocpCore>(); // 하나의 IOCP 코어 공유
        _states.reserve(kClients);
        _services.reserve(kClients);

        for (int i = 0; i < kClients; ++i)
        {
            const std::string name = "User" + std::to_string(i + 1);
            auto session = MakeShared<ServerSession>(name);

            ClientServiceRef service = MakeShared<ClientService>(
                NetAddress(L"127.0.0.1", 7777),
                _core,
                [session]() { return session; },
                1);

            ASSERT_CRASH(service->Start());
            _services.push_back(service);

            ClientState st;
            st.sess = session;
            st.period = duration<double>(1.0 / (kRps > 0.0 ? kRps : 1.0));
            const auto now = steady_clock::now();
            st.nextChat = now + milliseconds(10 + (i % 100)); // 살짝만 분산
            _states.push_back(std::move(st));
        }

        _running = true;
        _worker = std::thread([this]() { RunLoop(); });
    }

    void Stop()
    {
        _running = false;
        if (_worker.joinable()) _worker.join();
    }

    void PumpOnce()
    {
        if (_core) _core->Dispatch();
    }

private:
    struct ClientState
    {
        std::shared_ptr<ServerSession>          sess;
        std::chrono::duration<double>           period{ 1.0 };
        std::chrono::steady_clock::time_point   nextChat{};
        bool                                    roomReqInFlight = false;
    };

    std::string RandomString()
    {
        static const char charset[] =
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789";

        thread_local static std::mt19937 rng{ std::random_device{}() };
        thread_local static std::uniform_int_distribution<> distChar(0, sizeof(charset) - 2);
        thread_local static std::uniform_int_distribution<> distLen(10, 20); // 길이 10~20

        const int length = distLen(rng);

        std::string result;
        result.reserve(length);
        for (int i = 0; i < length; ++i)
            result.push_back(charset[distChar(rng)]);
        return result;
    }

    void RunLoop()
    {
        while (_running)
        {
            const auto now = steady_clock::now();

            for (int i = 0; i < (int)_states.size(); ++i)
            {
                auto& st = _states[i];
                auto& sess = st.sess;
                if (!sess->isLoggedIn) continue;

                const int userId = i + 1;

                // 아직 룸이 없고, 요청 미발송이면 1회성 방생성/조인
                if (!sess->isInRoom && !st.roomReqInFlight)
                {
                    if (userId <= kRooms) {
                        Protocol::C_CREATE_ROOM_REQ pkt;
                        pkt.set_roomname("Room_" + std::to_string(userId));
                        pkt.set_type(Protocol::ROOM_GROUP);
                        sess->Send(ServerPacketHandler::MakeSendBuffer(pkt));
                    }
                    else {
                        const int targetRoom = ((userId - 1) / 50) + 1; // 50명/방
                        Protocol::C_JOIN_ROOM_REQ pkt;
                        pkt.set_roomid(targetRoom);
                        sess->Send(ServerPacketHandler::MakeSendBuffer(pkt));
                    }
                    st.roomReqInFlight = true;
                    continue;
                }

                // 입장 완료되면 인플라이트 해제
                if (sess->isInRoom) st.roomReqInFlight = false;

                // 방 안이면 1Hz로 채팅
                if (sess->isInRoom && now >= st.nextChat)
                {
                    Protocol::C_ROOM_CHAT_REQ pkt;
                    pkt.set_message(RandomString()); // 10~20글자 랜덤
                    sess->Send(ServerPacketHandler::MakeSendBuffer(pkt));
                    st.nextChat = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(st.period);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

private:
    IocpCoreRef                          _core;
    std::vector<ClientServiceRef>        _services;
    std::vector<ClientState>             _states;
    std::thread                          _worker;
    std::atomic<bool>                    _running{ false };
};

// === 종료 핸들러 ===
static std::atomic<bool> g_running = true;
static void SigIntHandler(int) { g_running = false; }

// === main ===
int main()
{
    std::signal(SIGINT, SigIntHandler);
    ServerPacketHandler::Init();

    std::cout << "[Client] Fixed profile: clients=" << kClients
        << " rooms=" << kRooms
        << " rps=" << kRps << std::endl;

    ClientSimulator sim;
    sim.Start();

    while (g_running)
    {
        sim.PumpOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    sim.Stop();
    return 0;
}
