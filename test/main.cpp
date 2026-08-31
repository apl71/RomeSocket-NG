#include <iostream>
#include <liburing.h>
#include "log.hpp"

int main()
{
    io_uring ring{};

    int ret = io_uring_queue_init(256, &ring, 0);

    if (ret < 0) {
        std::cerr << "Failed to initialize io_uring\n";
        return 1;
    }

    std::cout << "io_uring initialized!\n";

    ROME_LOG("test log");

    io_uring_queue_exit(&ring);
    return 0;
}