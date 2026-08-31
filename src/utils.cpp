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