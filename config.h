#ifndef CONFIG_H
#define CONFIG_H

#include <atomic>

#define PAGE_SIZE 4096
#define ROW_SIZE 100

#define PAGE
// #define RECORD

#define time_out (10 * 1000) // us


enum lock_t { W, R, VAILD, NONE };

#define SERVER_HOST "10.11.6.117:50049"


class Status {
public:
    Status() { }

    void Init() {
        last_succ_count_ = 0;
        succ_count_ = 0;
        last_abort_count_ = 0;
        abort_count_ = 0;

        last_dds_count_ = 0;
        dds_count_ = 0;
        last_dds_dura_ = 0;
        dds_dura_ = 0;
    }

    void Check() {
        last_succ_count_.store(succ_count_);
        last_abort_count_.store(abort_count_);
        last_dds_dura_.store(dds_dura_);
        last_dds_count_.store(dds_count_);
    }

    uint64_t Throughtput() {
        uint64_t res = succ_count_.load() - last_succ_count_.load();
        return res;
    }

    uint64_t AbortCount() {
        uint64_t res = abort_count_.load() - last_abort_count_.load();
        return res;
    }

    uint64_t DDSDura() {
        uint64_t dura = dds_dura_.load() - last_dds_dura_.load();
        return dura;
    }

    uint64_t DDSCount() {
        uint64_t res = dds_count_.load() - last_dds_count_.load();
        return res;
    }

    std::atomic_uint64_t last_succ_count_;
    std::atomic_uint64_t succ_count_;
    std::atomic_uint64_t last_abort_count_;
    std::atomic_uint64_t abort_count_;

    std::atomic_uint64_t last_dds_count_;
    std::atomic_uint64_t dds_count_;
    std::atomic_uint64_t last_dds_dura_;
    std::atomic_uint64_t dds_dura_;

    uint64_t last_time_;
};


class ServerStatus {
public:
    ServerStatus() { }

    void Init() {
        rpc_count_atomic_ = 0;
        last_rpc_count_atomic_ = 0;
        dds_dura_ = 0;
        last_dds_dura_ = 0;
        abort_count_ = 0;
        last_abort_count_ = 0;
    }

    void Check() {
        last_dds_dura_.store(dds_dura_);
        last_rpc_count_atomic_.store(rpc_count_atomic_);
        last_abort_count_.store(abort_count_);
    }

    uint64_t DDSDura() {
        uint64_t dura = dds_dura_.load() - last_dds_dura_.load();
        return dura;
    }

    uint64_t RPCCount() {
        uint64_t count = rpc_count_atomic_.load() - last_rpc_count_atomic_.load();
        return count;
    }

    double AbortRatio() {
        double abort = abort_count_.load() - last_abort_count_.load();
        double rpc = rpc_count_atomic_.load() - last_rpc_count_atomic_.load();
        double ratio = 0;
        if (rpc != 0) {
            ratio = abort / rpc;
        }
        return ratio;
    }

    std::atomic_uint64_t rpc_count_atomic_;
    std::atomic_uint64_t last_rpc_count_atomic_;
    std::atomic_uint64_t abort_count_;
    std::atomic_uint64_t last_abort_count_;
    std::atomic_uint64_t last_dds_dura_;
    std::atomic_uint64_t dds_dura_;
};


#endif