#include <system_error>

#include "iouring_backend.hpp"
#include "log.hpp"

namespace rome::uring {

Backend::Backend():
    Backend(DEFAULT_RING_SIZE) {}

Backend::Backend(unsigned ring_size) {
    int ret = io_uring_queue_init(ring_size, &_ring, 0);
    if (ret != 0) {
        ROME_LOG("Fail to initialize io_uring. Error: {}", ret);
        throw std::system_error(-ret, std::generic_category(), "io_uring_queue_init");
    }
}

SubmitResult Backend::submit(std::unique_ptr<IOEvent> event) {
    if (!event || !event->conn) {
        ROME_LOG("Fail to submit the event because `event` is a null pointer.");
        return SubmitResult::NULL_EVENT;
    }

    IOEvent *raw_event = event.get();
    if (raw_event->type == IOType::RECV || raw_event->type == IOType::SEND) {
        auto rs_event = static_cast<RSEvent *>(raw_event);
        if (rs_event->offset > rs_event->buffer.size() ||
            rs_event->remaining > rs_event->buffer.size() - rs_event->offset) {
            ROME_LOG(
                "Fail to submit the event: offset = {}, remaining = {}.",
                rs_event->offset,
                rs_event->remaining
            );
            return SubmitResult::SIZE_OVERFLOW;
        }
    }
    _pending_events.try_emplace(raw_event, std::move(event));

    io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    if (!sqe) {
        _pending_events.erase(raw_event);
        ROME_LOG("Fail to submit the event: sqe == nullptr.");
        return SubmitResult::SQE_UNAVAILABLE;
    }
    switch (raw_event->type) {
        case IOType::ACCEPT:
            submit_accept(sqe, static_cast<AcceptEvent *>(raw_event));
            break;
        case IOType::RECV:
            submit_recv(sqe, static_cast<RSEvent *>(raw_event));
            break;
        case IOType::SEND:
            submit_send(sqe, static_cast<RSEvent *>(raw_event));
            break;
    }

    io_uring_sqe_set_data(sqe, raw_event);

    int ret = io_uring_submit(&_ring);
    if (ret < 0) {
        ROME_LOG("Fatal error: Fail to submit the event, io_uring_submit() returns {}", ret);
        throw std::system_error(-ret, std::generic_category(), "io_uring_submit");
    }
    return SubmitResult::SUCCESS;
}

void Backend::submit_accept(io_uring_sqe *sqe, AcceptEvent *event) {
    io_uring_prep_accept(
        sqe,
        event->listen_fd,
        reinterpret_cast<sockaddr *>(&event->conn->client_addr),
        &event->conn->client_addr_len,
        0
    );
}

void Backend::submit_recv(io_uring_sqe *sqe, RSEvent *event) {
    io_uring_prep_recv(
        sqe,
        event->conn->fd,
        event->buffer.data() + event->offset,
        event->remaining,
        0
    );
}

void Backend::submit_send(io_uring_sqe *sqe, RSEvent *event) {
    io_uring_prep_send(
        sqe,
        event->conn->fd,
        event->buffer.data() + event->offset,
        event->remaining,
        0
    );
}

std::unique_ptr<IOEvent> Backend::wait() {
    io_uring_cqe *cqe = nullptr;

    int ret = io_uring_wait_cqe(&_ring, &cqe);
    if (ret < 0) {
        ROME_LOG("Fail to wait for cqe: {}", -ret);
        return nullptr;
    }
    // get unique_ptr back here with its raw pointer
    IOEvent *raw_event = static_cast<IOEvent *>(io_uring_cqe_get_data(cqe));
    int result = cqe->res;
    io_uring_cqe_seen(&_ring, cqe);
    if (!raw_event) {
        ROME_LOG("Fail to get IOEvent: user data is empty.");
        return nullptr;
    }
    auto iter = _pending_events.find(raw_event);
    if (iter == _pending_events.end()) {
        ROME_LOG("Fail to get IOEvent: not in the pending queue.");
        return nullptr;
    }
    auto event = std::move(iter->second);
    _pending_events.erase(iter);
    event->res = result;
    return event;
}

Backend::~Backend() {
    io_uring_queue_exit(&_ring);
}

} // namespace rome::uring