#pragma once
#include "pch.h"                 // [1] 항상 최상단
#include "Session.h"
#include "ServerPacketHandler.h"
#include "Protocol.pb.h"
#include <string>
#include <atomic>

class ServerSession : public PacketSession
{
public:
    explicit ServerSession(const std::string& name_ = "") : name(name_) {}

    // per-client state
    std::string name;
    std::atomic<bool> isLoggedIn{ false };
    std::atomic<bool> isInRoom{ false };
    std::atomic<int32> roomId{ -1 };

    // 로그만 하던 소멸자 제거 (기본 소멸자 사용)
    //~ServerSession() override {}

    void OnConnected() override
    {
        Protocol::C_LOGIN_REQ pkt;
        pkt.set_name(name);
        Send(ServerPacketHandler::MakeSendBuffer(pkt));
    }

    void OnRecvPacket(BYTE* buffer, int32 len) override
    {
        auto self = GetPacketSessionRef();
        ServerPacketHandler::HandlePacket(self, buffer, len);
    }

    // 로그만 하던 OnSend 제거
    // void OnSend(int32 len) override {}

    void OnDisconnected() override
    {
        isLoggedIn = false;
        isInRoom = false;
        roomId = -1;
    }
};


