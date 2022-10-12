#include <iostream>
#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include <random>
#include <thread>
#include "lock_service.h"
#include "instance.h"

using namespace std;

int HOT_PERCENT = 20;   // (10%)

int gen_num(default_random_engine& e) {
    int space = PAGE_SIZE / ROW_SIZE;
    std::uniform_int_distribution<int> u1(0, space / (100 / HOT_PERCENT) - 1);
    std::uniform_int_distribution<int> u2(space / (100 / HOT_PERCENT), space - 1);

    int num = e();
    if (num % 100 < HOT_PERCENT) {
        return u1(e);
    }
    else {
        return u2(e);
    }
}

void start_server(int port, Instance* ins) {
    brpc::Server server;
    MasterServiceImpl mas_srv;
    mas_srv.instance_ = ins;
    if (server.AddService(&mas_srv, brpc::SERVER_OWNS_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
    }

    brpc::ServerOptions options;
    options.idle_timeout_sec = -1;
    string host = ins->local_ip_ + ":" + to_string(port);

    if (server.Start(host.c_str(), &options) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        assert(false);
        // return -1;
    }

    ins->srv_has_start_ = true;

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    // this_thread::sleep_for(chrono::seconds(3));
    server.RunUntilAskedToQuit();

}


void count_thread(Status* status) {
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> last = std::chrono::system_clock::now();
    while (1) {
        std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> curr = std::chrono::system_clock::now();
        if (std::chrono::duration_cast<std::chrono::nanoseconds>(curr - last) < std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::nanoseconds(1000000000))) {
            this_thread::sleep_for(chrono::nanoseconds(1000000000 - std::chrono::duration_cast<std::chrono::nanoseconds>(curr - last).count()));
        }
        else {
            uint64_t throughtput = status->Throughtput();
            cout << "throughtput: " << throughtput << " ops/s" << endl;
            uint64_t abort_count = status->AbortCount();
            cout << "abort      : " << abort_count << " ops/s" << endl;
            uint64_t latency = 0;
            if ((status->DDSCount() + status->AbortCount()) > 0) { latency = status->DDSDura() / (status->DDSCount() + status->AbortCount()) / 1000; }
            cout << "avg latency: " << latency << " us." << endl;

            // cout << status->DDSDura() << endl;
            // cout << status->DDSCount() << endl;
            // cout << status->AbortCount() << endl;

            status->Check();
            last = curr;
        }
    }
}

// ./client 1，即在 ip = local_host, port = 50050 + 1 上启动 MasterServer
int main(int argc, char** argv) {
    if (argc <= 1) assert(false);
    int ins_id = atoi(&argv[1][0]);
    int port = ins_id + 50050;

    Instance* instance = new Instance(ins_id);
    instance->ins_port_ = port;
    instance->GetLocalIP();
    std::thread t(start_server, port, instance);
    while (!instance->srv_has_start_) {}
    instance->BuildConn();


    default_random_engine e;
    e.seed(time(0));

    Status status;
    status.Init();
    instance->status_ = &status;

    std::thread t2(count_thread, &status);


    int count = 0;
    while (1) {
        int res = 0;
        int res_unlck = 0;
        int num = gen_num(e);
        assert(num < PAGE_SIZE / ROW_SIZE);

#ifdef PAGE 
        res = instance->Lock();
        if (!res) { instance->UnLock(); }
#elif defined(RECORD)
        res = instance->Lock(num);
        if (!res) { res_unlck = instance->UnLock(num); }
#endif
        assert(!res_unlck);
        if (!res) { status.succ_count_.fetch_add(1); }

        // if (count % 1000000 == 0) { cout << count << endl; }
        count++;
    }

    return 0;
}