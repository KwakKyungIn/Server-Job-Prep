// Metrics.h  — REPORT-READY METRICS (v2)
#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>

/* 메트릭스 관례
   - 모든 카운터 증감은 memory_order_relaxed 권장 (성능)
   - *_ops    : IOCP 완료 개수(커널 이벤트 단위)
   - *_packets: 앱 레벨 패킷 개수(파서 단위)
   - *_bytes  : 바이트 합계
   - *_gauge  : 현재 값(게이지)
   - *_peak   : 1초 내 최대값
*/

struct Metrics {

#define packets_recv io_recv_ops
#define packets_sent io_send_ops

    /* ========== 네트워크(IOCP/소켓) ========== */
    // IO 완료 개수(커널 기준) — WSASend/WSARecv 완료 당 1 증가
    std::atomic<uint64_t> io_recv_ops{ 0 };
    std::atomic<uint64_t> io_send_ops{ 0 };
    std::atomic<uint64_t> io_accept_ops{ 0 };

    // 앱 레벨 패킷(파서 기준) — HandlePacket 진입/브로드캐스트 전송 기준
    std::atomic<uint64_t> app_packets_in{ 0 };
    std::atomic<uint64_t> app_packets_out{ 0 };

    // 바이트 합계(소켓 레벨)
    std::atomic<uint64_t> bytes_recv{ 0 };
    std::atomic<uint64_t> bytes_sent{ 0 };

    // 연결 상태
    std::atomic<uint64_t> connections_opened{ 0 };   // OnConnected
    std::atomic<uint64_t> connections_closed{ 0 };   // OnDisconnected
    std::atomic<uint32_t> connections_gauge{ 0 };    // 현재 살아있는 세션 수
    std::atomic<uint32_t> connections_peak{ 0 };     // 최근 1초 최대 동시 연결

    /* ========== 브로드캐스트/룸 ========== */
    // 브로드캐스트 실제 전달 수(= 수신자 수 단위) — 부하 스케일 판단에 중요
    std::atomic<uint64_t> broadcast_deliveries{ 0 };

    // 룸/유저 게이지
    std::atomic<uint32_t> rooms_gauge{ 0 };          // 현재 생성된 방 수
    std::atomic<uint32_t> rooms_peak{ 0 };           // 최근 1초 최대 방 수
    std::atomic<uint32_t> players_gauge{ 0 };        // 현재 접속 중(로그인 완료) 인원
    std::atomic<uint32_t> players_peak{ 0 };

    // 룸 채팅 결과
    std::atomic<uint64_t> room_enter_ok{ 0 };
    std::atomic<uint64_t> room_enter_fail{ 0 };
    std::atomic<uint64_t> room_leave{ 0 };

    /* ========== JobQueue ========== */
    std::atomic<uint64_t> jobs_enqueued{ 0 };
    std::atomic<uint64_t> jobs_executed{ 0 };
    std::atomic<uint32_t> jobqueue_gauge{ 0 };       // 현재 큐 길이(추적 가능하면)
    std::atomic<uint32_t> jobqueue_peak{ 0 };        // 최근 1초 피크

    /* ========== 풀/메모리(전송버퍼, 메모리풀) ========== */
    // SendBufferChunk 전역 풀 입출력/사용량
    std::atomic<uint64_t> sendbuf_global_push{ 0 };
    std::atomic<uint64_t> sendbuf_global_pop{ 0 };
    std::atomic<uint32_t> sendbuf_inuse_gauge{ 0 };  // 현재 대여중 청크 수
    std::atomic<uint32_t> sendbuf_inuse_peak{ 0 };

    // Object/MemoryPool 통계(선택적으로 갱신)
    std::atomic<uint64_t> mpool_alloc_total{ 0 };
    std::atomic<uint64_t> mpool_free_total{ 0 };
    std::atomic<uint32_t> mpool_inuse_gauge{ 0 };
    std::atomic<uint32_t> mpool_inuse_peak{ 0 };

    /* ========== DB ========== */
    std::atomic<uint32_t>  conn_pool_size{ 0 };
    std::atomic<uint64_t>  conn_pool_pop_total{ 0 };
    std::atomic<uint64_t>  conn_pool_push_total{ 0 };
    std::atomic<uint64_t>  conn_pool_acquire_wait_ms_total{ 0 };
    std::atomic<uint64_t>  conn_acquire_fail{ 0 };

    std::atomic<uint64_t>  db_pop_waits{ 0 };
    std::atomic<uint64_t>  db_query_count{ 0 };
    std::atomic<uint64_t>  db_prepare_fail{ 0 };
    std::atomic<uint64_t>  db_exec_ok{ 0 };
    std::atomic<uint64_t>  db_exec_fail{ 0 };
    std::atomic<uint64_t>  db_fetch_ok{ 0 };
    std::atomic<uint64_t>  db_fetch_no_data{ 0 };
    std::atomic<uint64_t>  db_fetch_fail{ 0 };
    std::atomic<uint64_t>  db_unbind_calls{ 0 };

    // 채팅 로그(파일/DB) 결과
    std::atomic<uint64_t>  room_log_open_fail{ 0 };
    std::atomic<uint64_t>  chat_log_file_lines{ 0 };
    std::atomic<uint64_t>  chat_log_db_ok{ 0 };
    std::atomic<uint64_t>  chat_log_db_fail{ 0 };

    /* ========== 지연(Histogram: server-side broadcast path) ========== */
    // 서버가 C_ROOM_CHAT_REQ 수신~룸 Broadcast()에서 모든 대상에게 Send() 호출 완료까지의 시간(us)
    static constexpr int   LAT_BUCKETS = 12;
    // 경계(us): 10,20,50,100,200,500,1k,2k,5k,10k,20k,50k, +overflow
    static constexpr uint32_t LAT_BOUNDS[LAT_BUCKETS] = {
        10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000
    };
    std::atomic<uint64_t>  lat_buckets[LAT_BUCKETS + 1]{}; // 마지막은 overflow
    std::atomic<uint64_t>  lat_count{ 0 };
    std::atomic<uint64_t>  lat_sum_us{ 0 }; // 평균 계산용

    inline void ObserveBroadcastLatencyUS(uint32_t us) {
        lat_sum_us.fetch_add(us, std::memory_order_relaxed);
        lat_count.fetch_add(1, std::memory_order_relaxed);
        int idx = 0;
        while (idx < LAT_BUCKETS && us > LAT_BOUNDS[idx]) ++idx;
        lat_buckets[idx].fetch_add(1, std::memory_order_relaxed);
    }

    /* ========== 1초 단위 출력 ========== */
    void TickAndPrint1s() {
        static uint64_t pr_io = 0, ps_io = 0;
        static uint64_t pin = 0, pout = 0;
        static uint64_t br = 0, bs = 0, bd = 0;
        static uint64_t je = 0, jx = 0, dqc = 0, dpw = 0;
        static uint64_t push = 0, pop = 0, mp_alloc = 0, mp_free = 0;

        const auto nr_io = io_recv_ops.load();
        const auto ns_io = io_send_ops.load();
        const auto nin = app_packets_in.load();
        const auto nout = app_packets_out.load();
        const auto nbr = bytes_recv.load();
        const auto nbs = bytes_sent.load();
        const auto nbd = broadcast_deliveries.load();
        const auto nje = jobs_enqueued.load();
        const auto njx = jobs_executed.load();
        const auto pqp = jobqueue_peak.load();
        const auto connG = connections_gauge.load();
        const auto connP = connections_peak.load();
        const auto roomsG = rooms_gauge.load();
        const auto roomsP = rooms_peak.load();
        const auto plyG = players_gauge.load();
        const auto plyP = players_peak.load();
        const auto ndqc = db_query_count.load();
        const auto ndpw = db_pop_waits.load();
        const auto sb_in = sendbuf_inuse_gauge.load();
        const auto sb_pk = sendbuf_inuse_peak.load();
        const auto sb_push = sendbuf_global_push.load();
        const auto sb_pop = sendbuf_global_pop.load();
        const auto nmpAlloc = mpool_alloc_total.load();
        const auto nmpFree = mpool_free_total.load();
        const auto mpG = mpool_inuse_gauge.load();
        const auto mpP = mpool_inuse_peak.load();

        // 1줄: 핵심 트래픽
        std::printf(
            "[1s] io r/s=%llu s/s=%llu | app in/s=%llu out/s=%llu | bytes r/s=%llu s/s=%llu | bc-deliv/s=%llu\n",
            (unsigned long long)(nr_io - pr_io), (unsigned long long)(ns_io - ps_io),
            (unsigned long long)(nin - pin), (unsigned long long)(nout - pout),
            (unsigned long long)(nbr - br), (unsigned long long)(nbs - bs),
            (unsigned long long)(nbd - bd)
        );

        // 2줄: 큐 & 연결/룸 게이지
        std::printf(
            "     jobs e/s=%llu x/s=%llu (q-peak=%u) | conns now=%u (peak=%u) | rooms now=%u (peak=%u) | players now=%u (peak=%u)\n",
            (unsigned long long)(nje - je), (unsigned long long)(njx - jx), pqp,
            connG, connP, roomsG, roomsP, plyG, plyP
        );

        // 3줄: DB & 풀 상태
        std::printf(
            "     db q/s=%llu waits/s=%llu | sendbuf inuse=%u (peak=%u) push/s=%llu pop/s=%llu | mpool inuse=%u (peak=%u) alloc/s=%llu free/s=%llu\n",
            (unsigned long long)(ndqc - dqc), (unsigned long long)(ndpw - dpw),
            sb_in, sb_pk, (unsigned long long)(sb_push - push), (unsigned long long)(sb_pop - pop),
            mpG, mpP, (unsigned long long)(nmpAlloc - mp_alloc), (unsigned long long)(nmpFree - mp_free)
        );

        // 4줄: 지연(평균/중상위 버킷만)
        const auto lcnt = lat_count.load();
        const auto lsum = lat_sum_us.load();
        if (lcnt > 0) {
            const double avg = double(lsum) / double(lcnt);
            const auto b_100 = lat_buckets[3].load();   // <=100us
            const auto b_500 = lat_buckets[5].load();   // <=500us
            const auto b_1ms = lat_buckets[6].load();   // <=1ms
            const auto b_5ms = lat_buckets[8].load();   // <=5ms
            const auto b_50p = lat_buckets[LAT_BUCKETS].load(); // >50ms
            std::printf("     latency(us) avg=%.1f | <=100:%llu <=500:%llu <=1ms:%llu <=5ms:%llu >50ms:%llu\n",
                avg,
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

        // 1초 피크 게이지 리셋
        jobqueue_peak.store(0, std::memory_order_relaxed);
        connections_peak.store(connG, std::memory_order_relaxed);
        rooms_peak.store(roomsG, std::memory_order_relaxed);
        players_peak.store(plyG, std::memory_order_relaxed);
        sendbuf_inuse_peak.store(sb_in, std::memory_order_relaxed);
        mpool_inuse_peak.store(mpG, std::memory_order_relaxed);

        // 지연은 누적 유지(보고서용 총계/평균/히스토리)
    }
};

extern Metrics GMetrics;
