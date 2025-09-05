#ifndef NOMINMAX
#define NOMINMAX 1
#endif
// DummyClient/Main/DummyClient.cpp
#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferReader.h"
#include "ServerPacketHandler.h"
#include "Protocol.pb.h"
#include <algorithm> // std::min, std::max, std::clamp
#include <atomic>
#include <csignal>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <random>
#include <vector>
#include <chrono>
#include <functional>
#include <cstdlib>

// ---- clamp fallback for pre-C++17 ----
#if (__cplusplus < 201703L) && (!defined(_MSVC_LANG) || _MSVC_LANG < 201703L)
template <typename T>
constexpr T clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }
#else
using std::clamp;
#endif

// 새로 추가
#include "ServerSession.h"

// === 콘솔 안전 출력 ===
std::mutex g_coutMtx;
#define TS_COUT(expr) do { if (g_verbose) { std::lock_guard<std::mutex> _lk(g_coutMtx); std::cout << expr << std::endl; } } while(0)

// ===== CLI & 시나리오 =====
struct LoadProfile {
    int   clients = 500;
    int   rooms = 10;
    double rps = 5.0;   // per client
    bool  verbose = false;
    std::string mode = "load"; // smoke|load|broadcast
};

static bool g_verbose = false;

static LoadProfile ParseArgs(int argc, char** argv) {
    LoadProfile p;
    // 기본값: "load"
    for (int i = 1; i < argc; i++) {
        std::string a(argv[i]);
        auto next = [&](int& dst) { if (i + 1 < argc) dst = std::atoi(argv[++i]); };
        auto nextd = [&](double& dst) { if (i + 1 < argc) dst = std::atof(argv[++i]); };
        if (a == "--mode" && i + 1 < argc) { p.mode = argv[++i]; }
        else if (a == "--clients") { next(p.clients); }
        else if (a == "--rooms") { next(p.rooms); }
        else if (a == "--rps") { nextd(p.rps); }
        else if (a == "--verbose" && i + 1 < argc) {
            int v = std::atoi(argv[++i]); p.verbose = (v != 0);
        }
    }

    // 모드별 기본값 오버라이드
    if (p.mode == "smoke") {
        // 10–50명, 1 msg/s, 5분 정도
        if (p.clients == 500) p.clients = 50;
        p.rooms = clamp(p.rooms, 1, 5); // 1~5개면 충분
        p.rps = 1.0;
    }
    else if (p.mode == "broadcast") {
        // 100명/방 × 10방 = 1000명, 2 msg/s
        if (p.clients == 500) p.clients = 1000;
        if (p.rooms == 10)  p.rooms = 10;
        p.rps = 2.0;
    }
    else {
        // load (기본부하)
        // 10방 × 50명 = 500명, 5 msg/s
        p.mode = "load";
        // 인자 주면 그대로 쓰고, 아니면 디폴트 유지
    }

    g_verbose = p.verbose;
    return p;
}

// ===== 페이로드 생성: T1 타임스탬프 포함 + 크기 믹스 =====
static std::string RandomHex8(std::mt19937& rng) {
    static const char* hex = "0123456789abcdef";
    uint32_t v = rng();
    std::string s(8, '0');
    for (int i = 7; i >= 0; --i) { s[i] = hex[v & 0xF]; v >>= 4; }
    return s;
}

static std::string MakePayload(const std::string& name, size_t targetSize, std::mt19937& rng) {
    using namespace std::chrono;
    int64_t t1 = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    std::string head = "T1=" + std::to_string(t1) + "|name=" + name + "|id=" + RandomHex8(rng) + "|";
    std::string s = head;
    if (s.size() < targetSize) s.resize(targetSize, 'x');
    return s;
}

static size_t PickPayloadSize(std::mt19937& rng) {
    // 80% 64B, 18% 256B, 2% 1~2KB
    std::uniform_int_distribution<int> d(1, 100);
    int r = d(rng);
    if (r <= 80) return 64;
    if (r <= 98) return 256;
    std::uniform_int_distribution<int> spike(1024, 2048);
    return static_cast<size_t>(spike(rng));
}

// 전역: 전체 시뮬레이터에서 방 생성 예산
static std::atomic<int> g_createBudget{ 10 };

// === ClientSimulator ===
class ClientSimulator
{
public:
    explicit ClientSimulator(const LoadProfile& prof) : _profile(prof) {
        g_createBudget.store(_profile.rooms);
    }

    void Start()
    {
        _core = MakeShared<IocpCore>(); // 하나의 IOCP 코어 공유

        _states.reserve(_profile.clients);
        _services.reserve(_profile.clients);

        for (int i = 0; i < _profile.clients; i++)
        {
            std::string name = "User" + std::to_string(i + 1);
            auto session = MakeShared<ServerSession>(name);

            ClientServiceRef service = MakeShared<ClientService>(
                NetAddress(L"127.0.0.1", 7777),
                _core,                               // <-- 공유 코어
                [session]() { return session; },
                1);

            ASSERT_CRASH(service->Start());

            _services.push_back(service);

            ClientState st;
            st.sess = session;
            st.rps = _profile.rps;
            st.period = std::chrono::duration<double>(1.0 / std::max<double>(0.001, st.rps));
            auto now = std::chrono::steady_clock::now();
            // 초기화 (바로 행동 가능)
            st.nextJoinAt = now;                  // 바로 1회 시도 허용
            st.nextCreateAt = now;                // 바로 1회 시도 허용
            st.roomReqInFlight = false;           // 아직 요청 없음
            st.nextChat = now + std::chrono::milliseconds(10 + (i % 100)); // 약간의 초기 지터
            st.lastCreate = now - std::chrono::seconds(120);
            st.lastJoin = now - std::chrono::seconds(60);
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

    // 메인에서 IOCP 한 번만 펌프
    void PumpOnce()
    {
        if (_core) _core->Dispatch();
    }

private:
    struct ClientState
    {
        std::shared_ptr<ServerSession> sess;
        // 채팅 전송 스케줄
        double rps = 1.0;
        std::chrono::duration<double> period;
        std::chrono::steady_clock::time_point nextChat;

        // 조인/생성 쿨다운 기준용
        std::chrono::steady_clock::time_point lastCreate;
        std::chrono::steady_clock::time_point lastJoin;

        bool roomReqInFlight = false; // 방 생성/조인 요청 보냈으면 true (ACK 오면 해제)
        std::chrono::steady_clock::time_point nextJoinAt;
        std::chrono::steady_clock::time_point nextCreateAt;

        // RNG 초기화 수정(선택): this 기반 말고 시간+랜덤 디바이스로
        std::mt19937 rng{
            static_cast<uint32_t>(
                std::chrono::high_resolution_clock::now().time_since_epoch().count()
            ) ^ std::random_device{}()
        };
    };

    static bool CanDo(const std::chrono::steady_clock::time_point& last,
        std::chrono::milliseconds cd,
        const std::chrono::steady_clock::time_point& now)
    {
        return (now - last) >= cd;
    }

    void RunLoop()
    {
        using namespace std::chrono;

        const auto CD_CREATE = seconds(60);
        const auto CD_JOIN = seconds(5); // 빠르게 조인되도록 단축
        static auto startAt = steady_clock::now(); // 방 준비시간 게이팅

        std::uniform_int_distribution<int> creatorChance(1, 100);

        while (_running)
        {
            auto now = steady_clock::now();

            for (auto& st : _states)
            {
                auto& sess = st.sess;
                if (!sess->isLoggedIn) continue;

                // (1) 아직 방없으면: 조인 우선, 일부만 방 생성 (예산 기반)
                if (!sess->isInRoom)
                {
                    // 방 준비시간 2초 게이트
                    if (now - startAt < seconds(2)) {
                        continue;
                    }

                    // ACK 대기중이면 다시 안보냄 (스팸 방지)
                    if (st.roomReqInFlight) {
                        continue;
                    }

                    // --- 방 생성 (아주 일부만) ---
                    if (creatorChance(st.rng) <= 2 && now >= st.nextCreateAt)
                    {
                        int old = g_createBudget.load(std::memory_order_relaxed);
                        while (old > 0 && !g_createBudget.compare_exchange_weak(
                            old, old - 1, std::memory_order_acq_rel, std::memory_order_relaxed)) {
                            // retry
                        }
                        if (old > 0) {
                            Protocol::C_CREATE_ROOM_REQ pkt;
                            pkt.set_roomname("Room_" + sess->name);
                            pkt.set_type(Protocol::ROOM_GROUP);
                            sess->Send(ServerPacketHandler::MakeSendBuffer(pkt));

                            st.roomReqInFlight = true;                 // ⇦ 인플라이트 진입
                            st.nextCreateAt = now + seconds(60);       // 하드 쿨다운
                            TS_COUT("[" << sess->name << "] Create room (budget left=" << g_createBudget.load() << ")");
                            continue;
                        }
                    }

                    // --- 방 조인 ---
                    if (now >= st.nextJoinAt)
                    {
                        Protocol::C_JOIN_ROOM_REQ pkt;
                        int rid = 1 + (st.rng() % std::max<int>(1, _profile.rooms));
                        pkt.set_roomid(rid);
                        sess->Send(ServerPacketHandler::MakeSendBuffer(pkt));

                        st.roomReqInFlight = true;           // ⇦ 인플라이트 진입
                        st.nextJoinAt = now + seconds(5);    // 하드 쿨다운
                        TS_COUT("[" << sess->name << "] Join room #" << rid);
                    }
                    continue;
                }
                else {
                    // 방에 들어오면 인플라이트 해제 (ACK 경로가 늦게 와도 안전)
                    st.roomReqInFlight = false;
                }
                // (2) 방 안에 있는 경우: 목표 rps에 맞춰 채팅 전송
                if (now >= st.nextChat)
                {
                    size_t sz = PickPayloadSize(st.rng);
                    std::string msg = MakePayload(sess->name, sz, st.rng);

                    Protocol::C_ROOM_CHAT_REQ pkt;
                    pkt.set_message(msg);
                    sess->Send(ServerPacketHandler::MakeSendBuffer(pkt));

                    // 다음 시간(약간의 지터 추가)
                    std::uniform_real_distribution<double> jitter(0.9, 1.1);
                    st.nextChat = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(st.period * jitter(st.rng));
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

private:
    LoadProfile _profile;
    IocpCoreRef _core; // 공유 IOCP 코어
    std::vector<ClientServiceRef> _services;
    std::vector<ClientState> _states;
    std::thread _worker;
    std::atomic<bool> _running{ false };
};

// === 전역 종료 플래그 ===
std::atomic<bool> g_running = true;
void SigIntHandler(int) {
    g_running = false;
    if (g_verbose) {
        std::lock_guard<std::mutex> _lk(g_coutMtx);
        std::cout << "\nShutdown signal received: exiting..." << std::endl;
    }
}

// === main ===
int main(int argc, char** argv)
{
    std::signal(SIGINT, SigIntHandler);
    ServerPacketHandler::Init();

    LoadProfile prof = ParseArgs(argc, argv);

    // 요약 출력
    {
        std::lock_guard<std::mutex> _lk(g_coutMtx);
        std::cout << "[Client] mode=" << prof.mode
            << " clients=" << prof.clients
            << " rooms=" << prof.rooms
            << " rps=" << prof.rps
            << " verbose=" << (prof.verbose ? 1 : 0)
            << std::endl;
    }

    ClientSimulator simulator(prof);
    simulator.Start();

    while (g_running)
    {
        simulator.PumpOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    simulator.Stop();
    return 0;
}
