// DummyClient/Main/DummyClient.cpp  — minimal fixed-profile version


#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "ServerPacketHandler.h"
#include "Protocol.pb.h"
#include "ServerSession.h"
#include "ThreadManager.h"
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
static constexpr double kRps =3.0;   // per client

// === 시뮬레이터 ===
class ClientSimulator
{
public:
    void Start()
    {
        _core = MakeShared<IocpCore>(); // 하나의 IOCP 코어 공유
        _states.reserve(kClients);
        _services.reserve(kClients);

        const auto now = steady_clock::now();

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
            st.nextChat = (steady_clock::time_point::max)();

            const int userId = i + 1;
            if (userId <= kRooms) {
                // 10명: 1초 후 방 생성
                st.action = RoomAction::CreateAfterWait;
                st.actionTime = now + seconds(1);
            }
            else {
                // 490명: 2초 후 방 조인
                st.action = RoomAction::JoinAfterWait;
                st.actionTime = now + seconds(2);
            }

            _states.push_back(std::move(st));
        }

        _running = true;

        unsigned int hc = std::thread::hardware_concurrency();
        int ioThreads = (hc > 0 ? static_cast<int>(hc) / 4 : 1);
        if (ioThreads < 1) ioThreads = 1;

        GThreadManager->Launch([this]() {
            while (_running) {
                _core->Dispatch();
            }
            });

        GThreadManager->Launch([this]() {
            RunLoopPartition(0, 1); // tid=0, tcount=1 => 전체 순회
            });
    }

    void Stop()
    {
        _running = false;
        GThreadManager->Join();  // ThreadManager가 관리하는 모든 워커 join
    }

    void PumpOnce()
    {
        if (_core) _core->Dispatch();
    }

private:
    enum class RoomAction { None, CreateAfterWait, JoinAfterWait };

    struct ClientState
    {
        std::shared_ptr<ServerSession>          sess;
        std::chrono::duration<double>           period{ 1.0 };
        std::chrono::steady_clock::time_point   nextChat{};
        bool                                    roomReqInFlight = false;

        RoomAction                               action = RoomAction::None;
        std::chrono::steady_clock::time_point    actionTime{};
        bool                                     joinedOnce = false; // 첫 입장 감지
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

    void RunLoopPartition(int tid, int tcount)
    {
        while (_running)
        {
            const auto now = steady_clock::now();

            for (int i = tid; i < static_cast<int>(_states.size()); i += tcount)  // 자기 파티션만 처리
            {
                auto& st = _states[i];
                auto& sess = st.sess;
                if (!sess->isLoggedIn) continue;

                const int userId = i + 1;

                // 스케줄된 시각에 1회성 방 생성/조인 요청
                if (!sess->isInRoom && !st.roomReqInFlight && st.action != RoomAction::None && now >= st.actionTime)
                {
                    if (st.action == RoomAction::CreateAfterWait) {
                        Protocol::C_CREATE_ROOM_REQ pkt;
                        pkt.set_roomname("Room_" + std::to_string(userId)); // 1~10번이 각 방 생성
                        pkt.set_type(Protocol::ROOM_GROUP);
                        sess->Send(ServerPacketHandler::MakeSendBuffer(pkt));
                    }
                    else { // JoinAfterWait
                        const int targetRoom = ((userId - 1) / 50) + 1; // 50명/방, 1~10번 방으로 분산
                        Protocol::C_JOIN_ROOM_REQ pkt;
                        pkt.set_roomid(targetRoom);
                        sess->Send(ServerPacketHandler::MakeSendBuffer(pkt));
                    }
                    st.roomReqInFlight = true;
                    st.action = RoomAction::None; // 재전송 방지
                    continue;
                }

                // 입장 완료되면 인플라이트 해제 + 채팅 시작 타이밍 초기화(버스트 방지)
                if (sess->isInRoom) {
                    st.roomReqInFlight = false;

                    // ☆ 채팅 타이머 최초 가동(arming): max()로 막아둔 걸 풀어준다
                    if (st.nextChat == (steady_clock::time_point::max)()) {
                        st.nextChat = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(st.period);
                    }
                }

                // 방 안이면 kRps 주기로 채팅
                if (sess->isInRoom && now >= st.nextChat)
                {
                    int maxBurst = 10; // catch-up 캡 (기존 로직 유지)
                    do {
                        Protocol::C_ROOM_CHAT_REQ pkt;
                        pkt.set_message(RandomString()); // 10~20글자 랜덤
                        sess->Send(ServerPacketHandler::MakeSendBuffer(pkt));
                        st.nextChat += std::chrono::duration_cast<std::chrono::steady_clock::duration>(st.period);
                    } while (--maxBurst > 0 && now >= st.nextChat);
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }

private:
    IocpCoreRef                          _core;
    std::vector<ClientServiceRef>        _services;
    std::vector<ClientState>             _states;
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
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
   

    sim.Stop();
    return 0;
}
