#pragma once
#include "pch.h"                 // [1] 항상 최상단
#include "Session.h"
#include "ServerPacketHandler.h"
#include "Protocol.pb.h"
#include <string>
#include <atomic>
#include <iostream>
#include <mutex>




extern std::mutex g_coutMtx;
#define TS_COUT(expr) do { std::lock_guard<std::mutex> _lk(g_coutMtx); std::cout << expr << std::endl; } while(0)

class ServerSession : public PacketSession
{
public:
    explicit ServerSession(const std::string& name_ = "") : name(name_) {}

    // per-client state
    std::string name;
    std::atomic<bool> isLoggedIn{ false };
    std::atomic<bool> isInRoom{ false };
    std::atomic<int32> roomId{ -1 };

    ~ServerSession() override { TS_COUT("~ServerSession " << name); }

    void OnConnected() override
    {
        TS_COUT("[" << name << "] Connected. Sending login...");
        Protocol::C_LOGIN_REQ pkt;
        pkt.set_name(name);
        Send(ServerPacketHandler::MakeSendBuffer(pkt));
    }

    void OnRecvPacket(BYTE* buffer, int32 len) override
    {
        auto self = GetPacketSessionRef();
        ServerPacketHandler::HandlePacket(self, buffer, len);
    }

    void OnSend(int32 len) override
    {
        TS_COUT("[" << name << "] Sent " << len << " bytes");
    }

    void OnDisconnected() override
    {
        TS_COUT("[" << name << "] Disconnected");
        isLoggedIn = false;
        isInRoom = false;
        roomId = -1;
    }
};
