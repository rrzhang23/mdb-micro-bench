#include <iostream>
#include <thread>
#include "instance.h"
#include "config.h"

row_t::row_t() { }

void Instance::Init() {
    srv_has_start_ = false;
#ifdef PAGE
    page_.id = 0;
    page_.lock = lock_t::VAILD;
#elif defined(RECORD)
    row_size_ = PAGE_SIZE / ROW_SIZE;
    for (int i = 0; i < row_size_; i++) {
        rows_[i] = new row_t();
        rows_[i]->id = i;
        rows_[i]->lock = lock_t::VAILD;
        // std::cout << rows_[i]->id << ", rows_ size: " << rows_.size() <<std::endl;
    }
    Print();
#endif
}

Instance::Instance(int ins_id) {
    this->ins_id_ = ins_id;
    Init();
    Print();
}

int Instance::BuildConn() {
    this->channel_.Init(SERVER_HOST, &this->options_);
    this->stub_ = new DDSService_Stub(&channel_);

    BuildConnRequest request;
    BuildConnResponse response;
    brpc::Controller cntl;
    request.set_ins_id(ins_id_);
    request.set_ins_port(ins_port_);
    stub_->BuildConn(&cntl, &request, &response, NULL);

    if (!cntl.Failed()) {
        // LOG(INFO);
    }
    else {
        assert(false);
    }

    return 0;
}

void Instance::GetLocalIP() {
    DDSService_Stub* stub;
    brpc::ChannelOptions options;
    brpc::Channel channel;

    // channel.Init("127.0.0.1", this->ins_id_, &options);
    channel.Init(SERVER_HOST, &options);
    stub = new DDSService_Stub(&channel);

    EchoRequest request;
    EchoResponse response;
    brpc::Controller cntl;

    stub->Echo(&cntl, &request, &response, NULL);

    if (!cntl.Failed()) {
        this->local_ip_ = response.ip();
    }
    else {
        assert(false);
    }

    delete stub;
}

#ifdef PAGE
int Instance::Lock() {
    int status = 0;
    std::unique_lock<std::mutex> lck(page_.mtx);
    if (page_.lock == lock_t::VAILD) {
        char* buf;
        std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> start = std::chrono::system_clock::now();
        status = DDSLock(0, buf, PAGE_SIZE, this->ins_id_);
        std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> end = std::chrono::system_clock::now();
        status_->dds_dura_.fetch_add(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());

        delete buf;
        if (status == 0) {
            status_->dds_count_.fetch_add(1);
            page_.lock = lock_t::W;
        }
        else {
            status_->abort_count_.fetch_add(1);
            return status;
        }
    }
    else {
        assert(page_.lock != lock_t::VAILD);
        page_.lock = lock_t::W;
    }
    return status;
}

int Instance::UnLock() {
    std::unique_lock<std::mutex> lck(page_.mtx);
    assert(page_.lock != lock_t::VAILD);
    page_.lock = lock_t::NONE;
    page_.cv.notify_all();

    return 0;
}
int Instance::Valid(char* buf, int32_t size) {
    std::unique_lock<std::mutex> lck(page_.mtx);
    assert(page_.lock != lock_t::VAILD);
    if (page_.cv.wait_for(lck, std::chrono::microseconds(1000), [&]() { return (page_.lock == lock_t::NONE); })) {
        assert(page_.lock == lock_t::NONE);
        assert(page_.lock != lock_t::VAILD);
        assert(page_.lock != lock_t::W);
        assert(page_.lock != lock_t::R);
        assert(size == PAGE_SIZE);
        memcpy(buf, page_.value, size);
        page_.lock = lock_t::VAILD;
        return 0;
    }
    else {
        // std::cout << "file: " << __FILE__ << ", line: " << __LINE__ << std::endl;
        return 1;
    }
}

#elif defined(RECORD)
int Instance::Lock(uint64_t key) {
    int status = 0;
    row_t* row = rows_[key];
    assert(row != nullptr);
    std::unique_lock<std::mutex> lck(row->mtx);
    if (row->lock == lock_t::VAILD) {
        char* buf;
        std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> start = std::chrono::system_clock::now();
        status = DDSLock(key, buf, ROW_SIZE, this->ins_id_);
        std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> end = std::chrono::system_clock::now();
        status_->dds_dura_.fetch_add(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
        delete buf;
        if (status == 0) {
            status_->dds_count_.fetch_add(1);
            row->lock = lock_t::W;
        }
        else {
            status_->abort_count_.fetch_add(1);
            return status;
        }
    }
    else {
        assert(row->lock != lock_t::VAILD);
        row->lock = lock_t::W;
        return status;
    }
    return status;
}
int Instance::UnLock(uint64_t key) {
    row_t* row = rows_[key];
    assert(row != nullptr);
    std::unique_lock<std::mutex> lck(row->mtx);
    assert(row->lock == lock_t::W);
    row->lock = lock_t::NONE;
    row->cv.notify_all();
    return 0;
}
int Instance::Valid(uint64_t key, char* buf, int32_t size) {
    row_t* row = rows_[key];
    std::unique_lock<std::mutex> lck(row->mtx);
    assert(row->lock != lock_t::VAILD);
    if (row->cv.wait_for(lck, std::chrono::microseconds(1000), [&]() { return (row->lock == lock_t::NONE); })) {
        assert(row->lock != lock_t::VAILD);
        assert(row->lock != lock_t::W);
        assert(row->lock != lock_t::R);
        assert(size == ROW_SIZE);
        memcpy(buf, row->value, size);
        row->lock = lock_t::VAILD;
        return 0;
    }
    else {
        // std::cout << "file: " << __FILE__ << ", line: " << __LINE__ << std::endl;
        return 1;
    }
}
#endif


int Instance::DDSLock(uint64_t key, char*& buf, int size, int ins_id) {
    DDSLockRequest request;
    DDSLockResponse response;
    brpc::Controller cntl;
    int status = 0;
    buf = new char[size];

    request.set_key_id(key);
    request.set_lock_type(0);
    request.set_value_size(size);
    request.set_ins_id(ins_id);

    stub_->DDSLock(&cntl, &request, &response, NULL);

    status = response.status();

    if (!cntl.Failed()) {
        if (status == 0) {
            if (response.value_size() != size) { std::cout << "response.value_size(): " << response.value_size() << ", size: " << size << std::endl; }
            assert(response.value_size() == size);
            assert(response.value().size() == size);
            assert(buf);
            memcpy(buf, response.value().data(), size);
        }
    }
    else {
        assert(false);
    }

    return status;
}
