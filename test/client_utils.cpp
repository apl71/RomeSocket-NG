#include <thread>
#include <numeric>
#include <barrier>

#include "client_utils.hpp"
#include "utils.hpp"

int connect_server(const std::string &address, uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return sock;
    }
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    int ret = inet_pton(AF_INET, address.c_str(), &server_addr.sin_addr);
    if (ret <= 0) {
        return ret;
    }
    ret = connect(sock, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr));
    if (ret < 0) {
        return ret;
    }
    return 0;
}

size_t send_all(int sock, const void *data, size_t length) {
    const auto *buffer = static_cast<const char *>(data);
    size_t offset = 0;
    while (offset < length) {
        size_t sent = ::send(sock, buffer + offset, length - offset, 0);
        if (sent <= 0) {
            return offset;
        }
        offset += sent;
    }
    return offset;
}

size_t recv_all(int sock, void *data, size_t length) {
    size_t offset = 0;
    while (offset < length) {
        size_t received = ::recv(sock, data + offset, length - offset, 0);
        if (received <= 0) {
            return offset;
        }
        offset += received;
    }
    return offset;
}

BenchmarkRun run_benchmark(
    const BenchmarkConfig &config,
    const std::string &address,
    uint16_t port
) {
    BenchmarkRun result;
    switch (config.type) {
        case BenchmarkType::FIRST_REQUEST_LATENCY:
            result = collect_first_request_latency(config, address, port);
            break;
        default:
            break;
    }
    return result;
}

BenchmarkRun collect_first_request_latency(
    const BenchmarkConfig &config,
    const std::string &address,
    uint16_t port
) {
    BenchmarkRun result;
    // prepare threads
    std::vector<std::thread> threads;
    // store work result for a single thread
    struct alignas(64) WorkerResult {
        std::vector<uint64_t> timings;
        uint64_t attempted = 0;
        uint64_t succeeded = 0;
        uint64_t payload_bytes = 0;
    };

    std::vector<WorkerResult> worker_results(config.thread_num);
    for (auto &r : worker_results) {
        r.timings.reserve(config.test_num);
    }

    Stopwatch wall_watch;
    std::barrier start_barrier(config.thread_num + 1, [&]() {
        wall_watch.start();
    });
    std::barrier end_barrier(config.thread_num + 1, [&]() {
        result.wall_elapsed_ns = wall_watch.end();
    });

    for (int i = 0; i < config.thread_num; i++) {
        threads.emplace_back([&, thread_id = i](){
            // create buffer for testing
            // TODO: only work for echo server here
            std::vector<char> send_buf(config.payload_size, 0);
            std::vector<char> recv_buf(config.payload_size, 0);
            // start here
            start_barrier.arrive_and_wait();
            // test throughput
            for (int j = 0; j < config.test_num; j++) {
                worker_results[thread_id].attempted++;
                int sock = 0;
                Stopwatch watch;
                watch.start();
                sock = connect_server(address, port);
                if (sock < 0) {
                    continue;
                }
                
                bool success = true;

                size_t size = send_all(sock, send_buf.data(), config.payload_size);
                if (size != config.payload_size) {
                    success = false;
                }
                if (success) {
                    size = recv_all(sock, recv_buf.data(), config.payload_size);
                    if (size != config.payload_size) {
                        success = false;
                    }
                }

                if (success) {
                    auto elapsed_ns = watch.end();
                    worker_results[thread_id].succeeded++;
                    worker_results[thread_id].payload_bytes += config.payload_size;
                    worker_results[thread_id].timings.push_back(elapsed_ns);
                }

                close(sock);
            }
            // end timing
            end_barrier.arrive_and_wait();
        });
    }

    start_barrier.arrive_and_wait();

    end_barrier.arrive_and_wait();

    for (auto &thread: threads) {
        thread.join();
    }

    for (size_t i = 0; i < config.thread_num; i++) {
        for (auto &record: worker_results[i].timings) {
            result.time_recorder_ns.push_back(record);
        }
        result.attempted     += worker_results[i].attempted;
        result.succeeded     += worker_results[i].succeeded;
        result.payload_bytes += worker_results[i].payload_bytes;
    }
    result.config = config;
    return result;
}