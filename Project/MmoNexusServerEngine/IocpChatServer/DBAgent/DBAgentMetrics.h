#pragma once

#include <cstdint>

namespace DBAgentMetrics
{
    class ScopedRequestMetrics
    {
    public:
        explicit ScopedRequestMetrics(std::uint16_t packetId);
        ~ScopedRequestMetrics();

        ScopedRequestMetrics(const ScopedRequestMetrics&) = delete;
        ScopedRequestMetrics& operator=(const ScopedRequestMetrics&) = delete;

    private:
        std::uint8_t _op = 0;
        std::uint8_t _previousOp = 0;
        std::uint64_t _startUs = 0;
    };

    void Initialize();
    void Shutdown();

    void ObservePoolState(std::int64_t poolSize, std::int64_t poolInUse);
    void ObservePoolWait(double elapsedSeconds);
    void ObserveQueryDuration(double elapsedSeconds);
}
