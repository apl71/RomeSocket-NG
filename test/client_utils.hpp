#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int connect_server(const std::string &address, uint16_t port);

size_t send_all(int sock, const void *data, size_t length);

size_t recv_all(int sock, void *data, size_t length);

// timing the latency between launching the connection
// and receving the first response
// return total valid test num
int collect_first_request_latency(
    TimingRecorder &records,
    const std::string &name,
    const std::string &address,
    uint64_t port,
    uint64_t payload_size,
    int thread_num,
    int test_num
);

struct PerfReport;

// generate statistical report
// data will be divided by `record.name`
// report concludes:
//  1. throughput in Mbps
//  2. message rate in messages/s
//  3. latency in miliseconds, with p50 and p99
void benchmark(TimingRecorder &records);