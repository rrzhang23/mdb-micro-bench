#ifndef INSTNACE_H
#define INSTNACE_H

#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <map>
#include <brpc/channel.h>

#include "lock_service.pb.h"

#include "config.h"


class row_t {
public:
    row_t();
    uint64_t id;
    std::mutex mtx;
    lock_t lock;
    std::condition_variable cv;
#ifdef PAGE
    char value[PAGE_SIZE];
#elif defined(RECORD)
    char value[ROW_SIZE];
#endif
};

using page_t = row_t;

class Instance {
public:
    Instance() {};
    Instance(int ins_id);
    ~Instance() {};

    void Init();
    int BuildConn();
    void GetLocalIP();

#ifdef PAGE
    int Lock();
    int UnLock();
    int Valid(char* buf, int32_t size);
#elif defined(RECORD)
    int Lock(uint64_t row_id);
    int UnLock(uint64_t row_id);
    int Valid(uint64_t row_id, char* buf, int32_t size);
#endif

    int DDSLock(uint64_t key, char*& buf, int size, int ins_id);

    // private:
#ifdef PAGE
    // std::mutex page_mutex_;
    page_t page_;
#elif defined(RECORD)
    std::unordered_map<uint64_t, row_t*> rows_;
    int row_size_;
#endif
    int ins_id_;
    std::string local_ip_;
    int ins_port_;
    bool srv_has_start_;

    DDSService_Stub* stub_;
    brpc::ChannelOptions options_;
    brpc::Channel channel_;

    Status* status_;

    void Print() {
#ifdef RECORD
        std::cout << "ins_id: " << ins_id_ << ", rows_ size: " << rows_.size() << ", srv_has_start_: " << srv_has_start_ << std::endl;
#endif
    }
};

#endif

#ifdef PAGE
#elif defined(RECORD)
#endif