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

void benchmark(
    const std::string &name,
    const std::string &out_file_path,
    int sock,
    uint64_t payload_size,
    int thread_num
) {
    std::string md;
    md += "================================ Benchmark at ";
    md += current_time();
    md += " ================================";
    // test throughput
}