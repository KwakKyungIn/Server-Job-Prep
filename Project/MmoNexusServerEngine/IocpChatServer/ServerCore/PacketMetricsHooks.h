#pragma once

#include "Types.h"

namespace PacketMetricsHooks
{
    enum class FailureReason : uint8
    {
        Parse = 0,
        Validate = 1,
        Handler = 2,
    };

    using DispatchHook = void(*)(const char* handlerName, uint16 packetId);
    using HandledHook = void(*)(const char* handlerName, uint16 packetId, double elapsedSeconds);
    using FailureHook = void(*)(const char* handlerName, uint16 packetId, FailureReason reason);
    using PacketObjectHook = void(*)(const char* handlerName, uint16 packetId, const void* packetObject);

    inline DispatchHook& DispatchSlot()
    {
        static DispatchHook hook = nullptr;
        return hook;
    }

    inline HandledHook& HandledSlot()
    {
        static HandledHook hook = nullptr;
        return hook;
    }

    inline FailureHook& FailureSlot()
    {
        static FailureHook hook = nullptr;
        return hook;
    }

    inline PacketObjectHook& MakeSendSlot()
    {
        static PacketObjectHook hook = nullptr;
        return hook;
    }

    inline PacketObjectHook& ParsedSlot()
    {
        static PacketObjectHook hook = nullptr;
        return hook;
    }

    inline void SetHooks(
        DispatchHook dispatchHook,
        HandledHook handledHook,
        FailureHook failureHook,
        PacketObjectHook makeSendHook,
        PacketObjectHook parsedHook)
    {
        DispatchSlot() = dispatchHook;
        HandledSlot() = handledHook;
        FailureSlot() = failureHook;
        MakeSendSlot() = makeSendHook;
        ParsedSlot() = parsedHook;
    }

    inline void ClearHooks()
    {
        SetHooks(nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    inline void OnDispatch(const char* handlerName, uint16 packetId)
    {
        DispatchHook hook = DispatchSlot();
        if (hook)
            hook(handlerName, packetId);
    }

    inline void OnHandled(const char* handlerName, uint16 packetId, double elapsedSeconds)
    {
        HandledHook hook = HandledSlot();
        if (hook)
            hook(handlerName, packetId, elapsedSeconds);
    }

    inline void OnFailure(const char* handlerName, uint16 packetId, FailureReason reason)
    {
        FailureHook hook = FailureSlot();
        if (hook)
            hook(handlerName, packetId, reason);
    }

    inline void OnMakeSendBuffer(const char* handlerName, uint16 packetId, const void* packetObject)
    {
        PacketObjectHook hook = MakeSendSlot();
        if (hook)
            hook(handlerName, packetId, packetObject);
    }

    inline void OnPacketParsed(const char* handlerName, uint16 packetId, const void* packetObject)
    {
        PacketObjectHook hook = ParsedSlot();
        if (hook)
            hook(handlerName, packetId, packetObject);
    }
}
