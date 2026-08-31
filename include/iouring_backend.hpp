#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <liburing.h>
#include "connection.hpp"

namespace rome::uring {

constexpr unsigned DEFAULT_RING_SIZE = 1024;

enum class SubmitResult {
    SUCCESS,
    NULL_EVENT,
    SIZE_OVERFLOW,
    SQE_UNAVAILABLE,

    OTHER
};

struct Result {
    bool ok;
    std::string reason;
};

class Backend {

private:
    io_uring _ring;

    std::unordered_map<IOEvent *, std::unique_ptr<IOEvent>> _pending_events;

private:
    void submit_accept(io_uring_sqe *sqe, AcceptEvent *event);
    void submit_recv(io_uring_sqe *sqe, RSEvent *event);
    void submit_send(io_uring_sqe *sqe, RSEvent *event);

public:
    Backend();
    Backend(unsigned ring_size);

    SubmitResult submit(std::unique_ptr<IOEvent> event);
    std::unique_ptr<IOEvent> wait();

    ~Backend();
};


} // namespace rome::uring
