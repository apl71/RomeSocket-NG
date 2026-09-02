#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int connect_server(const std::string &address, uint16_t port);

// write markdown report to out_file_path
// report concludes:
//  1. throughput in Mbps
//  2. message rate in messages/s
//  3. latency in miliseconds, with p50 and p99
void benchmark(
    const std::string &name,
    const std::string &out_file_path,
    int sock,
    uint64_t payload_size,
    int thread_num
);

auto a = std::chrono::steady_clock::now();