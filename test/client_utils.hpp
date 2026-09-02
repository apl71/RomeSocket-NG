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

// write markdown report to out_file_path
// report concludes:
//  1. throughput in Mbps
//  2. message rate in messages/s
//  3. latency in miliseconds, with p50 and p99

// timing a send, save `test_num` results in `records`
// every thread will send `payload_size` bytes for `test_num` times
// every record in `records` notes the time of ONE thread sending
// and receving ONE `payload_size` bytes
void collect_echo_samples(
    TimingRecorder &records,
    const std::string &name,
    const std::string &address,
    uint64_t port,
    uint64_t payload_size,
    int thread_num,
    int test_num
);
