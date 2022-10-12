#ifndef LOCK_SERVER_H
#define LOCK_SERVER_H

#include <stdint.h>
#include <mutex>
#include <unordered_map>
#include <map>
#include <brpc/channel.h>
#include <condition_variable>
#include "lock_service.pb.h"
#include "config.h"


struct RPCPoint {
    brpc::ChannelOptions options_;
    brpc::Channel channel_;
    MasterService* stub_;
};

class row_item {
public:
    uint64_t id;
    int32_t instance_id;
    std::mutex row_mutex;
};

class LockServer {

public:
    LockServer();

    int BuildConn(int32_t target_id, std::string target);

    // private:
#ifdef PAGE
    int32_t page_owner_;
    std::mutex page_mutex_;
    std::condition_variable page_cv_;
#elif defined(RECORD)
    std::unordered_map<uint64_t, row_item*> rows_;
#endif

    std::map<int32_t, RPCPoint*> host_;
};


#endif