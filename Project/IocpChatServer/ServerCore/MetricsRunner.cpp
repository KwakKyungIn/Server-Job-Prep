// MetricsRunner.cpp
#include "pch.h"
#include "MetricsRunner.h"
#include "Metrics.h"
#include <atomic>
#include <thread>
#include <chrono>

static std::atomic<bool> g_run{ false };
static std::thread g_thr;

void StartMetricsTicker() {
    if (g_run.exchange(true)) return;
    g_thr = std::thread([] {
        while (g_run.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            GMetrics.TickAndPrint1s();
        }
        });
}

void StopMetricsTicker() {
    if (!g_run.exchange(false)) return;
    if (g_thr.joinable()) g_thr.join();
}
