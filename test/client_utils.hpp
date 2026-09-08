#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int connect_server(const std::string &address, uint16_t port);

size_t send_all(int sock, const void *data, size_t length);

size_t recv_all(int sock, void *data, size_t length);

// configuration for a single test
enum class BenchmarkType {
    // the time taken by `connect()`
    CONNECTION_LATENCY,
    // measure how much time does it cost
    // from `connect()` to receiving the first response
    // explain how quickly one gets first respond
    FIRST_REQUEST_LATENCY,
    // measure how much time does it cost
    // from `send()` to `recv()` after connecting
    // explain how long a ping-pong takes
    PERSISTENT_REQUEST_LATENCY,
    // test the maximum throughput of echo server
    ECHO_THROUGHPUT
};

struct BenchmarkConfig {
    std::string name;
    BenchmarkType type;

    size_t payload_size = 0;
    size_t thread_num = 1;
    size_t test_num = 0;
};

// origin data from a single test
struct BenchmarkRun {
    BenchmarkConfig config;

    std::vector<uint64_t> time_recorder_ns;

    uint64_t attempted = 0;
    uint64_t succeeded = 0;
    uint64_t payload_bytes = 0;
    uint64_t wall_elapsed_ns = 0;
};

BenchmarkRun run_benchmark(
    const BenchmarkConfig &config,
    const std::string &address,
    uint16_t port
);

// timing the latency between launching the connection
// and receving the first response
// return total valid test num
BenchmarkRun collect_first_request_latency(
    const BenchmarkConfig &config,
    const std::string &address,
    uint16_t port
);

struct GitInfo {
    std::string commit;
    std::string branch;
    bool dirty;
};

struct BuildInfo {
    std::string type;
    std::string compiler;
    std::string compiler_version;
    std::string cpp_standard;
};

struct SystemInfo {
    std::string os;
    std::string kernel;
    std::string architecture;
    std::string cpu;
    size_t logical_cpus;
    std::string environment;
};

struct BenchmarkContext {
    GitInfo git;
    BuildInfo build;
    SystemInfo system;
};

BenchmarkContext collect_benchmark_context();

void report_benchmark_result(
    const BenchmarkContext &context,
    const BenchmarkRun &result,
    const std::string &out_dir
);