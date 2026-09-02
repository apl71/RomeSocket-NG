#include <thread>

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

void collect_echo_samples(
    TimingRecorder &records,
    const std::string &name,
    const std::string &address,
    uint64_t port,
    uint64_t payload_size,
    int thread_num,
    int test_num
) {
    // prepare threads
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_num; i++) {
        threads.emplace_back([&](){
            // create buffer for testing
            // TODO: only work for echo server here
            std::vector<char> send_buf(payload_size, 0);
            std::vector<char> recv_buf(payload_size, 0);
            // test throughput
            for (int i = 0; i < test_num; i++) {
                ScopedTimer timer(name, records);
                int sock = connect_server(address, port);
                
                size_t size = send_all(sock, send_buf.data(), payload_size);
                if (size != payload_size) {
                    timer.cancel();
                    return;
                }
                size = recv_all(sock, recv_buf.data(), payload_size);
                if (size != payload_size) {
                    timer.cancel();
                    return;
                }
                close(sock);
            }
        });
    }
    for (auto &thread: threads) {
        thread.join();
    }
}