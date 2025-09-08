// Metrics.h  — REPORT-READY METRICS (v2 + DB pool print)
#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cmath>

struct Metrics {


#define packets_recv io_recv_ops
#define packets_sent io_send_ops

    /* ========== 네트워크(IOCP/소켓) ========== */
    std::atomic<uint64_t> io_recv_ops{ 0 };
    std::atomic<uint64_t> io_send_ops{ 0 };
    std::atomic<uint64_t> io_accept_ops{ 0 };

    // 앱 레벨 패킷
    std::atomic<uint64_t> app_packets_in{ 0 };
    std::atomic<uint64_t> app_packets_out{ 0 };

    // 바이트 합계
    std::atomic<uint64_t> bytes_recv{ 0 };
    std::atomic<uint64_t> bytes_sent{ 0 };

    // 연결 상태
    std::atomic<uint64_t> connections_opened{ 0 };
    std::atomic<uint64_t> connections_closed{ 0 };
    std::atomic<uint32_t> connections_gauge{ 0 };
    std::atomic<uint32_t> connections_peak{ 0 };

    /* ========== 브로드캐스트/룸 ========== */
    std::atomic<uint64_t> broadcast_deliveries{ 0 };
    std::atomic<uint32_t> rooms_gauge{ 0 };
    std::atomic<uint32_t> rooms_peak{ 0 };
    std::atomic<uint32_t> players_gauge{ 0 };
    std::atomic<uint32_t> players_peak{ 0 };

    // 룸 채팅 결과 (필요 시 사용)
    std::atomic<uint64_t> room_enter_ok{ 0 };
    std::atomic<uint64_t> room_enter_fail{ 0 };
    std::atomic<uint64_t> room_leave{ 0 };

    /* ========== JobQueue ========== */
    std::atomic<uint64_t> jobs_enqueued{ 0 };
    std::atomic<uint64_t> jobs_executed{ 0 };
    std::atomic<uint32_t> jobqueue_gauge{ 0 };
    std::atomic<uint32_t> jobqueue_peak{ 0 };

    /* ========== 풀/메모리(전송버퍼, 메모리풀) ========== */
    std::atomic<uint64_t> sendbuf_global_push{ 0 };
    std::atomic<uint64_t> sendbuf_global_pop{ 0 };
    std::atomic<uint32_t> sendbuf_inuse_gauge{ 0 };
    std::atomic<uint32_t> sendbuf_inuse_peak{ 0 };

    std::atomic<uint64_t> mpool_alloc_total{ 0 };
    std::atomic<uint64_t> mpool_free_total{ 0 };
    std::atomic<uint32_t> mpool_inuse_gauge{ 0 };
    std::atomic<uint32_t> mpool_inuse_peak{ 0 };

    /* ========== DB (풀 + 실행 통계) ========== */
    // 커넥션 풀
    std::atomic<uint32_t>  conn_pool_size{ 0 };
    std::atomic<uint64_t>  conn_pool_pop_total{ 0 };
    std::atomic<uint64_t>  conn_pool_push_total{ 0 };
    std::atomic<uint64_t>  conn_pool_acquire_wait_ms_total{ 0 };
    std::atomic<uint64_t>  conn_acquire_fail{ 0 };

    // (선택) 큐 대기 횟수/쿼리 수 집계용
    std::atomic<uint64_t>  db_pop_waits{ 0 };
    std::atomic<uint64_t>  db_query_count{ 0 };

    // 실행/페치 결과 (네 DBConnection.cpp에서 사용 중)
    std::atomic<uint64_t>  db_prepare_fail{ 0 };
    std::atomic<uint64_t>  db_exec_ok{ 0 };
    std::atomic<uint64_t>  db_exec_fail{ 0 };
    std::atomic<uint64_t>  db_fetch_ok{ 0 };
    std::atomic<uint64_t>  db_fetch_no_data{ 0 };
    std::atomic<uint64_t>  db_fetch_fail{ 0 };
    std::atomic<uint64_t>  db_unbind_calls{ 0 };

    // 채팅 로그 (필요 시)
    std::atomic<uint64_t>  room_log_open_fail{ 0 };
    std::atomic<uint64_t>  chat_log_file_lines{ 0 };
    std::atomic<uint64_t>  chat_log_db_ok{ 0 };
    std::atomic<uint64_t>  chat_log_db_fail{ 0 };

    /* ========== 지연(Histogram: server-side broadcast path) ========== */
    static constexpr int   LAT_BUCKETS = 12;
    static constexpr uint32_t LAT_BOUNDS[LAT_BUCKETS] = {
        10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000
    };
    std::atomic<uint64_t>  lat_buckets[LAT_BUCKETS + 1]{};
    std::atomic<uint64_t>  lat_count{ 0 };
    std::atomic<uint64_t>  lat_sum_us{ 0 };

    inline void ObserveBroadcastLatencyUS(uint32_t us) {
        lat_sum_us.fetch_add(us, std::memory_order_relaxed);
        lat_count.fetch_add(1, std::memory_order_relaxed);
        int idx = 0;
        while (idx < LAT_BUCKETS && us > LAT_BOUNDS[idx]) ++idx;
        lat_buckets[idx].fetch_add(1, std::memory_order_relaxed);
    }

    /* ========== 1초 단위 출력 ========== */
    void TickAndPrint1s() {
        // 이전 스냅샷
        static uint64_t pr_io = 0, ps_io = 0;
        static uint64_t pin = 0, pout = 0;
        static uint64_t br = 0, bs = 0, bd = 0;
        static uint64_t je = 0, jx = 0, dqc = 0, dpw = 0;
        static uint64_t push = 0, pop = 0, mp_alloc = 0, mp_free = 0;
        static uint64_t cp_pop = 0, cp_push = 0, cp_wait_ms = 0;

        // 현재 스냅샷
        const auto nr_io = io_recv_ops.load(std::memory_order_relaxed);
        const auto ns_io = io_send_ops.load(std::memory_order_relaxed);
        const auto nin = app_packets_in.load(std::memory_order_relaxed);
        const auto nout = app_packets_out.load(std::memory_order_relaxed);
        const auto nbr = bytes_recv.load(std::memory_order_relaxed);
        const auto nbs = bytes_sent.load(std::memory_order_relaxed);
        const auto nbd = broadcast_deliveries.load(std::memory_order_relaxed);

        const auto nje = jobs_enqueued.load(std::memory_order_relaxed);
        const auto njx = jobs_executed.load(std::memory_order_relaxed);
        const auto pqp = jobqueue_peak.load(std::memory_order_relaxed);

        const auto connG = connections_gauge.load(std::memory_order_relaxed);
        const auto connP = connections_peak.load(std::memory_order_relaxed); (void)connP; // 현재 라인에선 미사용

        const auto roomsG = rooms_gauge.load(std::memory_order_relaxed);
        const auto roomsP = rooms_peak.load(std::memory_order_relaxed); (void)roomsP;

        const auto plyG = players_gauge.load(std::memory_order_relaxed);
        const auto plyP = players_peak.load(std::memory_order_relaxed); (void)plyP;

        const auto ndqc = db_query_count.load(std::memory_order_relaxed);
        const auto ndpw = db_pop_waits.load(std::memory_order_relaxed);

        const auto sb_in = sendbuf_inuse_gauge.load(std::memory_order_relaxed);
        const auto sb_pk = sendbuf_inuse_peak.load(std::memory_order_relaxed);
        const auto sb_push = sendbuf_global_push.load(std::memory_order_relaxed);
        const auto sb_pop = sendbuf_global_pop.load(std::memory_order_relaxed);

        const auto nmpAlloc = mpool_alloc_total.load(std::memory_order_relaxed);
        const auto nmpFree = mpool_free_total.load(std::memory_order_relaxed);
        const auto mpG = mpool_inuse_gauge.load(std::memory_order_relaxed);
        const auto mpP = mpool_inuse_peak.load(std::memory_order_relaxed);

        const auto cpsize = conn_pool_size.load(std::memory_order_relaxed);
        const auto ncp_pop = conn_pool_pop_total.load(std::memory_order_relaxed);
        const auto ncp_push = conn_pool_push_total.load(std::memory_order_relaxed);
        const auto ncp_wait_ms = conn_pool_acquire_wait_ms_total.load(std::memory_order_relaxed);

        // 증분 계산
        const auto d_io_r = (unsigned long long)(nr_io - pr_io);
        const auto d_io_s = (unsigned long long)(ns_io - ps_io);
        const auto d_in = (unsigned long long)(nin - pin);
        const auto d_out = (unsigned long long)(nout - pout);
        const auto d_br = (unsigned long long)(nbr - br);
        const auto d_bs = (unsigned long long)(nbs - bs);
        const auto d_bd = (unsigned long long)(nbd - bd);

        const auto d_je = (unsigned long long)(nje - je);
        const auto d_jx = (unsigned long long)(njx - jx);

        const auto d_dqc = (unsigned long long)(ndqc - dqc);
        const auto d_dpw = (unsigned long long)(ndpw - dpw);

        const auto d_sb_push = (unsigned long long)(sb_push - push);
        const auto d_sb_pop = (unsigned long long)(sb_pop - pop);

        const auto d_mp_alloc = (unsigned long long)(nmpAlloc - mp_alloc);
        const auto d_mp_free = (unsigned long long)(nmpFree - mp_free);

        const auto d_cp_pop = (unsigned long long)(ncp_pop - cp_pop);
        const auto d_cp_push = (unsigned long long)(ncp_push - cp_push);
        const auto d_cp_waitms = (unsigned long long)(ncp_wait_ms - cp_wait_ms);
        const double acq_wait_ms_avg = (d_cp_pop > 0) ? (double)d_cp_waitms / (double)d_cp_pop : 0.0;

        // 1줄: 핵심 트래픽
        std::printf(
            "[1s] io r/s=%llu s/s=%llu | app in/s=%llu out/s=%llu | bytes r/s=%llu s/s=%llu | bc-deliv/s=%llu\n",
            d_io_r, d_io_s, d_in, d_out, d_br, d_bs, d_bd
        );

        // 2줄: 큐 & (게이지 피크는 별도 리셋)
        std::printf(
            "     jobs e/s=%llu x/s=%llu (q-peak=%u) |\n",
            d_je, d_jx, pqp
        );

        // 3줄: DB 풀 & 메모리 상태
        std::printf(
            "     db q/s=%llu waits/s=%llu | pool size=%u pop/s=%llu push/s=%llu acq-wait(ms)=%.1f | "
            "sendbuf inuse=%u (peak=%u) push/s=%llu pop/s=%llu | mpool inuse=%u (peak=%u) alloc/s=%llu free/s=%llu\n",
            d_dqc, d_dpw,
            cpsize, d_cp_pop, d_cp_push, acq_wait_ms_avg,
            sb_in, sb_pk, d_sb_push, d_sb_pop,
            mpG, mpP, d_mp_alloc, d_mp_free
        );




        // 4줄: 지연(평균/중상위 버킷)
        const auto lcnt = lat_count.load(std::memory_order_relaxed);
        const auto lsum = lat_sum_us.load(std::memory_order_relaxed);

        // === 1초 델타로 퍼센타일 계산 ===
        static uint64_t prev_lat_buckets[LAT_BUCKETS + 1] = {};
        static uint64_t prev_lat_count = 0;

        // 델타 버킷 계산
        uint64_t delta_buckets[LAT_BUCKETS + 1];
        uint64_t delta_total = lcnt - prev_lat_count;
        for (int i = 0; i <= LAT_BUCKETS; ++i) {
            const uint64_t cur = lat_buckets[i].load();
            delta_buckets[i] = (cur >= prev_lat_buckets[i]) ? (cur - prev_lat_buckets[i]) : 0;
            prev_lat_buckets[i] = cur;
        }
        prev_lat_count = lcnt;

        // 퍼센타일 헬퍼 (선형 보간)
        auto percentile_us = [&](double p)->uint32_t {
            if (delta_total == 0) return 0;

            const uint64_t target = static_cast<uint64_t>(std::ceil((p / 100.0) * double(delta_total)));
            uint64_t cum = 0;

            for (int i = 0; i <= LAT_BUCKETS; ++i) {
                const uint64_t c = delta_buckets[i];
                if (c == 0) continue;

                const uint64_t prevCum = cum;
                cum += c;

                if (cum >= target) {
                    const uint32_t lower = (i == 0) ? 0u : LAT_BOUNDS[i - 1];
                    const uint32_t upper = (i == LAT_BUCKETS)
                        ? (LAT_BOUNDS[LAT_BUCKETS - 1] * 2u)   // overflow 버킷 상한
                        : LAT_BOUNDS[i];
                    const uint64_t within = target - prevCum;
                    const double   frac = double(within) / double(c);
                    return static_cast<uint32_t>(lower + frac * (upper - lower));
                }
            }

            // Fallback: overflow 상한과 일치 (OOB 금지)
            return LAT_BOUNDS[LAT_BUCKETS - 1] * 2u;
            };

        const uint32_t p50 = percentile_us(50.0);
        const uint32_t p90 = percentile_us(90.0);
        const uint32_t p99 = percentile_us(99.0);


        if (lcnt > 0) {
            const double avg = double(lsum) / double(lcnt);
            const auto b_100 = lat_buckets[3].load(std::memory_order_relaxed);
            const auto b_500 = lat_buckets[5].load(std::memory_order_relaxed);
            const auto b_1ms = lat_buckets[6].load(std::memory_order_relaxed);
            const auto b_5ms = lat_buckets[8].load(std::memory_order_relaxed);
            const auto b_50p = lat_buckets[LAT_BUCKETS].load(std::memory_order_relaxed);
            std::printf("     latency(us) avg=%.1f | p50=%u p90=%u p99=%u | <=100:%llu <=500:%llu <=1ms:%llu <=5ms:%llu >50ms:%llu\n",
                avg, p50, p90, p99,
                (unsigned long long)b_100, (unsigned long long)b_500,
                (unsigned long long)b_1ms, (unsigned long long)b_5ms,
                (unsigned long long)b_50p);
        }

        // 스냅샷 업데이트
        pr_io = nr_io; ps_io = ns_io;
        pin = nin; pout = nout;
        br = nbr; bs = nbs; bd = nbd;
        je = nje; jx = njx; dqc = ndqc; dpw = ndpw;
        push = sb_push; pop = sb_pop; mp_alloc = nmpAlloc; mp_free = nmpFree;
        cp_pop = ncp_pop; cp_push = ncp_push; cp_wait_ms = ncp_wait_ms;

        // 1초 피크 게이지 리셋 (게이지는 현재값으로 덮어써서 '그 순간 피크' 보존)
        jobqueue_peak.store(0, std::memory_order_relaxed);
        connections_peak.store(connections_gauge.load(std::memory_order_relaxed), std::memory_order_relaxed);
        rooms_peak.store(rooms_gauge.load(std::memory_order_relaxed), std::memory_order_relaxed);
        players_peak.store(players_gauge.load(std::memory_order_relaxed), std::memory_order_relaxed);
        sendbuf_inuse_peak.store(sendbuf_inuse_gauge.load(std::memory_order_relaxed), std::memory_order_relaxed);
        mpool_inuse_peak.store(mpool_inuse_gauge.load(std::memory_order_relaxed), std::memory_order_relaxed);
        // 지연 히스토그램은 누적 유지
    }
};

extern Metrics GMetrics;
