#include <string>
#include <iostream>
#include <cstdint>

#include "client_utils.hpp"

constexpr std::string ADDRESS = "127.0.0.1";
constexpr uint16_t PORT = 8989;

// 64B, 1KB, 16KB, 64KB
constexpr uint64_t payload_sizes[4] = {
    64, 1024, 16 * 1024, 64 * 1024
};

int main() {
}