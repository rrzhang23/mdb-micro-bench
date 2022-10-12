#ifndef LOCK_SERVICE_H
#define LOCK_SERVICE_H



#include <gflags/gflags.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <brpc/channel.h>
#include <butil/endpoint.h>
#include <map>
#include <mutex>
#include <unordered_map>

#include "lock_service.pb.h"

class Instance;
class LockServer;
class ServerStatus;


class DDSServiceImpl : public DDSService {
public:
    DDSServiceImpl() {};
    virtual ~DDSServiceImpl() {};

    virtual void DDSLock(google::protobuf::RpcController* controller,
        const ::DDSLockRequest* request,
        ::DDSLockResponse* response,
        ::google::protobuf::Closure* done);

    virtual void Echo(::google::protobuf::RpcController* controller,
        const ::EchoRequest* request,
        ::EchoResponse* response,
        ::google::protobuf::Closure* done);

    virtual void BuildConn(::google::protobuf::RpcController* controller,
        const ::BuildConnRequest* request,
        ::BuildConnResponse* response,
        ::google::protobuf::Closure* done);

    // private:
    LockServer* dds_server_;
    ServerStatus* status_;
};

class MasterServiceImpl : public MasterService {
public:
    MasterServiceImpl() { }
    virtual ~MasterServiceImpl() {};

    virtual void Valid(::google::protobuf::RpcController* controller,
        const ::ValidRequest* request,
        ::ValidResponse* response,
        ::google::protobuf::Closure* done);

    virtual void Echo(::google::protobuf::RpcController* controller,
        const ::EchoRequest* request,
        ::EchoResponse* response,
        ::google::protobuf::Closure* done);
    // private:
    Instance* instance_;
};



#endif