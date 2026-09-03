#include <thread>
#include <numeric>

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

int collect_first_request_latency(
    TimingRecorder &recorder,
    const std::string &name,
    const std::string &address,
    uint64_t port,
    uint64_t payload_size,
    int thread_num,
    int test_num
) {
    // prepare threads
    std::vector<std::thread> threads;
    std::vector<TimingRecorder> local_recorders(thread_num);
    std::vector<int> valid_nums(thread_num, 0);
    for (int i = 0; i < thread_num; i++) {
        threads.emplace_back([&, thread_id = i](){
            // create buffer for testing
            // TODO: only work for echo server here
            std::vector<char> send_buf(payload_size, 0);
            std::vector<char> recv_buf(payload_size, 0);
            // write something for validation
            for (int j = 0; j < payload_size; j++) {
                send_buf[j] = static_cast<char>(j % 256);
            }
            // test throughput
            for (int j = 0; j < test_num; j++) {
                int sock = 0;
                bool success = true;
                {
                    ScopedTimer timer(name, local_recorders[thread_id]);
                    sock = connect_server(address, port);
                    
                    size_t size = send_all(sock, send_buf.data(), payload_size);
                    if (size != payload_size) {
                        timer.cancel();
                        success = false;
                    }
                    if (success) {
                        size = recv_all(sock, recv_buf.data(), payload_size);
                        if (size != payload_size) {
                            timer.cancel();
                            success = false;
                        }
                    }
                }
                close(sock);
                // validate
                if (memcmp(send_buf.data(), recv_buf.data(), payload_size) != 0) {
                    success = false;
                }
                if (success) {
                    valid_nums[thread_id]++;
                }
            }
        });
    }
    for (auto &thread: threads) {
        thread.join();
    }
    for (auto &local_recorder: local_recorders) {
        for (auto &record: local_recorder.get_records()) {
            recorder.record(record.name, record.elapsed_ns);
        }
    }
    return std::accumulate(valid_nums.begin(), valid_nums.end(), 0);
}