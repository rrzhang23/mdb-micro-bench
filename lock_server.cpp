

#include <cassert>
#include "lock_server.h"
#include "lock_service.pb.h"

LockServer::LockServer() {
#ifdef PAGE
    page_owner_ = -1;
#elif defined(RECORD)
    for (int i = 0; i < PAGE_SIZE / ROW_SIZE; i++) {
        rows_[i] = new row_item();
        rows_[i]->id = i;
        rows_[i]->instance_id = -1;
    }
#endif
}

int LockServer::BuildConn(int32_t target_id, std::string target) {
    assert(target_id >= 0);
    if (host_.find(target_id) != host_.end()) {
        LOG(INFO) << "dds to ins_" << target_id << " has exist";
        EchoRequest request;
        EchoResponse response;
        brpc::Controller cntl;
        host_[target_id]->stub_->Echo(&cntl, &request, &response, NULL);
        if (cntl.Failed()) {
            LOG(INFO) << "dds to ins_" << target_id << " broken";
            delete host_[target_id]->stub_;
            host_[target_id]->stub_ = new MasterService_Stub(&host_[target_id]->channel_);
            // assert(false);
        }
    }
    else {
        this->host_[target_id] = new RPCPoint();
        host_[target_id]->channel_.Init(target.c_str(), &(host_[target_id]->options_));
        host_[target_id]->stub_ = new MasterService_Stub(&host_[target_id]->channel_);
    }

    return 0;
}