// Metrics.h
#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>

struct Metrics {
    // 네트워크
    std::atomic<uint64_t> packets_recv{ 0 };
    std::atomic<uint64_t> packets_sent{ 0 };
    std::atomic<uint64_t> bytes_recv{ 0 };
    std::atomic<uint64_t> bytes_sent{ 0 };

    // 브로드캐스트(전송 대상 수 기준으로 세면 부하를 더 잘 반영)
    std::atomic<uint64_t> broadcast_deliveries{ 0 };

    // JobQueue
    std::atomic<uint64_t> jobs_enqueued{ 0 };
    std::atomic<uint64_t> jobs_executed{ 0 };
    std::atomic<uint32_t> jobqueue_peak{ 0 }; // 최근 1초 피크

    //풀 관련
    std::atomic<uint32_t> conn_pool_size;
    std::atomic<uint64_t> conn_pool_pop_total;
    std::atomic<uint64_t> conn_pool_push_total;
    std::atomic<uint64_t> conn_pool_acquire_wait_ms_total;
    std::atomic<uint64_t> conn_acquire_fail;

    // DB
    std::atomic<uint64_t> db_pop_waits{ 0 };
    std::atomic<uint64_t> db_query_count{ 0 };
    std::atomic<uint64_t> db_prepare_fail;
    std::atomic<uint64_t> db_exec_ok;
    std::atomic<uint64_t> db_exec_fail;
    std::atomic<uint64_t> db_fetch_ok;
    std::atomic<uint64_t> db_fetch_no_data;
    std::atomic<uint64_t> db_fetch_fail;
    std::atomic<uint64_t> db_unbind_calls;

   // Room 채팅 관련

    std::atomic<uint64_t> room_enter_ok;
    std::atomic<uint64_t> room_enter_fail;
    std::atomic<uint64_t> room_leave;
    std::atomic<uint64_t> room_log_open_fail;
    std::atomic<uint64_t> chat_log_file_lines;
    std::atomic<uint64_t> chat_log_db_ok;
    std::atomic<uint64_t> chat_log_db_fail;

    void TickAndPrint1s() {
        static uint64_t pr = 0, ps = 0, br = 0, bs = 0, bd = 0, je = 0, jx = 0, dpw = 0, dqc = 0;

        uint64_t nr = packets_recv.load();
        uint64_t ns = packets_sent.load();
        uint64_t nbr = bytes_recv.load();
        uint64_t nbs = bytes_sent.load();
        uint64_t nbd = broadcast_deliveries.load();
        uint64_t nje = jobs_enqueued.load();
        uint64_t njx = jobs_executed.load();
        uint32_t pqp = jobqueue_peak.load();
        uint64_t ndpw = db_pop_waits.load();
        uint64_t ndqc = db_query_count.load();

        std::printf(
            "[1s] pkts r/s=%llu s/s=%llu | bytes r/s=%llu s/s=%llu | bc-deliv/s=%llu | "
            "jobs e/s=%llu x/s=%llu (peak=%u) | db waits/s=%llu q/s=%llu\n",
            (unsigned long long)(nr - pr), (unsigned long long)(ns - ps),
            (unsigned long long)(nbr - br), (unsigned long long)(nbs - bs),
            (unsigned long long)(nbd - bd),
            (unsigned long long)(nje - je), (unsigned long long)(njx - jx),
            pqp,
            (unsigned long long)(ndpw - dpw), (unsigned long long)(ndqc - dqc)
        );

        pr = nr; ps = ns; br = nbr; bs = nbs; bd = nbd; je = nje; jx = njx; dpw = ndpw; dqc = ndqc;
        jobqueue_peak.store(0);
    }
};

extern Metrics GMetrics;
