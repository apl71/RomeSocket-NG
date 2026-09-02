#pragma once

#include <string>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <memory>
#include <mutex>

#include <sys/socket.h>
#include <netdb.h>

// convert ip address in string and port
// into sockaddr_storage and socklen_t
void make_address(
    const std::string &addr,
    uint16_t port,
    sockaddr_storage &storage,
    socklen_t &storage_len
);

template <typename To, typename From>
std::unique_ptr<To> unique_ptr_cast(
    std::unique_ptr<From> ptr)
{
    return std::unique_ptr<To>(
        static_cast<To*>(ptr.release())
    );
}

std::string current_time();

// timing class for benchmark
struct TimingRecord {
    std::string name;
    uint64_t elapsed_ns;
};

class TimingRecorder {
private:
    std::vector<TimingRecord> _records;
    std::mutex _mut;

public:
    TimingRecorder() = default;
    void record(const std::string &name, uint64_t elapsed_ns);
    std::vector<TimingRecord> get_records();
};

class ScopedTimer {
private:
    std::string_view _name;
    TimingRecorder &_recorder;
    std::chrono::steady_clock::time_point _start;
    bool _canceled;

public:
    ScopedTimer(std::string_view name, TimingRecorder &recorder);
    ~ScopedTimer();

    void cancel();
};