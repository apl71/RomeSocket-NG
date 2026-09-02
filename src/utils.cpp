#include <chrono>

#include "utils.hpp"

void make_address(
    const std::string &addr,
    uint16_t port,
    sockaddr_storage &storage,
    socklen_t &storage_len
) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo *result = nullptr;

    std::string service = std::to_string(port);

    int ret = getaddrinfo(
        addr.c_str(),
        service.c_str(),
        &hints,
        &result
    );

    if (ret != 0) {
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(ret));
    }

    std::memcpy(&storage, result->ai_addr, result->ai_addrlen);

    storage_len = static_cast<socklen_t>(result->ai_addrlen);

    freeaddrinfo(result);
}

std::string current_time() {
    using namespace std::chrono;

    const auto now = system_clock::now();

    const auto milliseconds = duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;

    const std::time_t time = system_clock::to_time_t(now);

    std::tm tm{};
    localtime_r(&time, &tm);

    return std::format(
        "[{:04}-{:02}-{:02} "
        "{:02}:{:02}:{:02}.{:03}]",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        milliseconds.count()
    );
}