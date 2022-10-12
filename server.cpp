#include <iostream>
#include <thread>
#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include "lock_server.h"
#include "lock_service.h"
using namespace std;

void count_thread(ServerStatus* status) {
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> last = std::chrono::system_clock::now();
    while (1) {
        std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> curr = std::chrono::system_clock::now();
        if (std::chrono::duration_cast<std::chrono::nanoseconds>(curr - last) < std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::nanoseconds(1000000000))) {
            this_thread::sleep_for(chrono::nanoseconds(1000000000 - std::chrono::duration_cast<std::chrono::nanoseconds>(curr - last).count()));
        }
        else {
            cout << "count atomic: " << status->RPCCount() << " ops/s" << endl;
            uint64_t latency = 0;
            if (status->RPCCount() > 0) { latency = status->DDSDura() / status->RPCCount() / 1000; }
            cout << "avg latency : " << latency << " us." << endl;
            cout << "abort ratio : " << status->AbortRatio() << endl; 

            status->Check();
            last = curr;
        }
    }

}

int main(int argc, char** argv) {
    bool is_server = true;
    // if (argc > 1) {
    //     if (argv[1][1] == 's') is_server = true;
    // }


    if (is_server) {
        ServerStatus status; status.Init();

        thread t(count_thread, &status);

        LockServer* ls = new LockServer();

        brpc::Server server;
        DDSServiceImpl dds_service_impl;
        dds_service_impl.status_ = &status;
        dds_service_impl.dds_server_ = ls;
        if (server.AddService(&dds_service_impl, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
            LOG(ERROR) << "Fail to add service";
            return -1;
        }

        brpc::ServerOptions options;
        options.idle_timeout_sec = -1;
        if (server.Start(SERVER_HOST, &options) != 0) {
            LOG(ERROR) << "Fail to start EchoServer";
            return -1;
        }

        // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
        server.RunUntilAskedToQuit();
    }
}