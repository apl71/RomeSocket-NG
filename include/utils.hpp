#pragma once

#include <string>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <memory>

#include <sys/socket.h>
#include <netdb.h>

// convert ip address in string and port
// into sockaddr_storage and socklen_t
void make_address(
    const std::string &addr,
    uint16_t port,
    sockaddr_storage &storage,
    socklen_t &storage_len
);

template <typename To, typename From>
std::unique_ptr<To> unique_ptr_cast(
    std::unique_ptr<From> ptr)
{
    return std::unique_ptr<To>(
        static_cast<To*>(ptr.release())
    );
}

std::string current_time();

auto start_timing();
// return time elapsed in milliseconds
auto end_timing();