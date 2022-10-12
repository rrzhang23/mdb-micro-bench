#include <mutex>
#include <thread>
#include <chrono>
#include "lock_service.h"
#include "config.h"
#include "instance.h"
#include "lock_server.h"

void DDSServiceImpl::DDSLock(::google::protobuf::RpcController* controller,
    const ::DDSLockRequest* request,
    ::DDSLockResponse* response,
    ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    ValidRequest v_request;
    ValidResponse v_response;
    brpc::Controller v_cntl;
    int size = request->value_size();
    char buf[size];


    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> start = std::chrono::system_clock::now();

    status_->rpc_count_atomic_.fetch_add(1);

#ifdef PAGE
    assert(size == PAGE_SIZE);
    assert(request->key_id() == 0);
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> start2 = std::chrono::system_clock::now();
    std::unique_lock<std::mutex> lck(dds_server_->page_mutex_);

    if (dds_server_->page_owner_ == -1) {
        dds_server_->page_owner_ = request->ins_id();
        response->set_value(buf, size);
        response->set_value_size(size);
        response->set_status(0);
        // std::cout << "owner change to: " << request->ins_id() << std::endl;
    }
    else {
        v_request.set_key_id(request->key_id());
        v_request.set_ins_id(request->ins_id());
        v_request.set_value_size(size);
        // std::cout << "file: " << __FILE__ << ", line: " << __LINE__ << std::endl;

        dds_server_->host_[dds_server_->page_owner_]->stub_->Valid(&v_cntl, &v_request, &v_response, NULL);
        std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> end2 = std::chrono::system_clock::now();

        if(std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2).count() > (time_out)) {
            status_->abort_count_.fetch_add(1);
        } 

        // std::cout << "file: " << __FILE__ << ", line: " << __LINE__ << std::endl;
        if (!v_cntl.Failed()) {
            // std::cout << "file: " << __FILE__ << ", line: " << __LINE__ << std::endl;
            response->set_status(v_response.status());
            if (v_response.status() == 0) {
                // std::cout << "v_response.key_id(): " << v_response.key_id() << ", v_response.value_size(): " << v_response.value_size()
                //     << ", v_response.status(): " << v_response.status() << std::endl;
                // std::cout << "owner change to: " << request->ins_id() << std::endl;

                if (v_response.status() != 0) assert(false);
                dds_server_->page_owner_ = request->ins_id();
                assert(v_response.value_size() == size);
                assert(v_response.value().size() == size);
                memcpy(buf, v_response.value().data(), size);

                response->set_value(buf, size);
                response->set_value_size(size);
                response->set_status(v_response.status());
                response->set_key_id(v_response.key_id());
            }
        }
        else {
            std::cout << "file: " << __FILE__ << ", line: " << __LINE__ << std::endl;
            // assert(false);
        }
    }
    // LOG(INFO) << "DDSLock from ins: " << request->ins_id() << ", key: " << request->key_id() << ", owner: " << dds_server_->page_owner_;


#elif defined(RECORD)
    assert(size == ROW_SIZE);
    row_item* row = dds_server_->rows_[request->key_id()];
    std::unique_lock<std::mutex> lck(row->row_mutex);
    // LOG(INFO) << "DDSLock from ins: " << request->ins_id() << ", key: " << request->key_id() << ", owner: " << row->instance_id;

    if (row->instance_id == -1) {
        row->instance_id = request->ins_id();
        response->set_value(buf, size);
        response->set_value_size(size);
        response->set_status(0);
        response->set_key_id(request->key_id());
    }
    else {
        v_request.set_key_id(request->key_id());
        v_request.set_ins_id(request->ins_id());
        v_request.set_value_size(size);
        // std::cout << __FILE__ << ", line: " << __LINE__ << std::endl;
        dds_server_->host_[row->instance_id]->stub_->Valid(&v_cntl, &v_request, &v_response, NULL);
        if (!v_cntl.Failed()) {
            response->set_status(v_response.status());
            if (v_response.status() == 0) {
                // std::cout << "v_response.key_id(): " << v_response.key_id() << ", v_response.value_size(): " << v_response.value_size()
                //     << ", v_response.status(): " << v_response.status() << std::endl;
                assert(v_response.value_size() == size);
                assert(v_response.value().size() == size);
                memcpy(buf, v_response.value().data(), size);

                response->set_value(buf, size);
                response->set_value_size(size);
                response->set_status(v_response.status());
                response->set_key_id(v_response.key_id());

                row->instance_id = request->ins_id();
            }
        }
    }
#endif


    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> end = std::chrono::system_clock::now();
    status_->dds_dura_.fetch_add(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
}

void DDSServiceImpl::Echo(::google::protobuf::RpcController* controller,
    const ::EchoRequest* request,
    ::EchoResponse* response,
    ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    response->set_ip(std::string(butil::ip2str(cntl->remote_side().ip).c_str()));
}

void DDSServiceImpl::BuildConn(::google::protobuf::RpcController* controller,
    const ::BuildConnRequest* request,
    ::BuildConnResponse* response,
    ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "Received request[log_id=" << cntl->log_id()
        << "] from " << cntl->remote_side()
        << " to " << cntl->local_side() << ", ins_id: " << request->ins_id() << ", ins_port: " << request->ins_port();

    butil::EndPoint ins_end;
    ins_end.ip = cntl->remote_side().ip;
    ins_end.port = request->ins_port();
    dds_server_->BuildConn(request->ins_id(), endpoint2str(ins_end).c_str());
}
















void MasterServiceImpl::Valid(::google::protobuf::RpcController* controller,
    const ::ValidRequest* request,
    ::ValidResponse* response,
    ::google::protobuf::Closure* done) {
    // LOG(INFO) << "Valid from ins: " << request->ins_id() << ", key: " << request->key_id();
    brpc::ClosureGuard done_guard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);
    char buf[request->value_size()];

#ifdef PAGE
    int status = instance_->Valid(buf, request->value_size());
#elif defined(RECORD)
    int status = instance_->Valid(request->key_id(), buf, request->value_size());
#endif

    response->set_status(status);
    if (status == 0) {
        response->set_key_id(request->key_id());
        response->set_value(buf, request->value_size());
        response->set_value_size(request->value_size());
    }
    // LOG(INFO) << "Valid from ins: " << request->ins_id() << ", key: " << request->key_id();
}

void MasterServiceImpl::Echo(::google::protobuf::RpcController* controller,
    const ::EchoRequest* request,
    ::EchoResponse* response,
    ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);
}