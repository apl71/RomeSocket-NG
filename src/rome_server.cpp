#include "rome_server.hpp"
#include "utils.hpp"
#include "log.hpp"

namespace rome::server {

RomeServer::RomeServer(const std::string &bind_addr, uint16_t port) : _port(port) {
    make_address(bind_addr, port, _bind_addr, _bind_addr_len);
}

rome::uring::SubmitResult RomeServer::submit_accept() {
    auto event = std::make_unique<AcceptEvent>();
    event->conn = std::make_shared<Connection>();
    event->listen_fd = _listen_fd;
    return _backend.submit(std::move(event));
    
}

rome::uring::SubmitResult RomeServer::submit_recv(int fd) {
    auto recv_event = std::make_unique<RSEvent>();
    auto iter = _connections.find(fd);
    if (iter == _connections.end()) {
        ROME_LOG("Fail to submit receive: fd is unavailable.");
        return rome::uring::SubmitResult::OTHER;
    }
    recv_event->conn = iter->second;
    recv_event->type = IOType::RECV;
    recv_event->remaining = MAX_BUFFER_SIZE;        // no fix-length protocal here
    return _backend.submit(std::move(recv_event));
}

void RomeServer::event_loop() {
    auto result = submit_accept();
    if (result != rome::uring::SubmitResult::SUCCESS) {
        ROME_LOG("Fail to submit accept io event, {}", static_cast<int>(result));
        return;
    }

    on_start();

    while (!_shutdown) {
        auto io_event = _backend.wait();
        if (!io_event || !io_event->conn) {
            ROME_LOG("Warning: empty event or suspending event received.");
            continue;
        }
        switch (io_event->type) {
            case IOType::ACCEPT:
                deal_accept(unique_ptr_cast<AcceptEvent>(std::move(io_event)));
                break;
            case IOType::RECV:
                deal_recv(unique_ptr_cast<RSEvent>(std::move(io_event)));
                break;
            case IOType::SEND:
                deal_send(unique_ptr_cast<RSEvent>(std::move(io_event)));
                break;
        }
    }

    on_shutdown();

    ROME_LOG("Server shutdown normally.");
    return;
}

void RomeServer::deal_accept(std::unique_ptr<AcceptEvent> event) {
    if (event->res < 0) {
        ROME_LOG("Fail to accept an connection: {}", -event->res);
        submit_accept();
        return;
    }
    int client_fd = event->res;
    event->conn->fd = client_fd;
    _connections[client_fd] = event->conn;
    // submit an recv for reading from new client
    submit_recv(client_fd);
    // submit a new accept request
    submit_accept();
}

void RomeServer::deal_recv(std::unique_ptr<RSEvent> event) {
    if (event->res == 0) {
        ROME_LOG("Peer close the connection.");
        close_connection(event->conn->fd);
        on_disconnect(std::move(event));
        return;
    } else if (event->res < 0) {
        ROME_LOG("Fail to receive, errno: {}", std::strerror(-event->res));
        close_connection(event->conn->fd);
        on_disconnect(std::move(event));
        return;
    }
    int fd = event->conn->fd;
    on_recv(std::move(event));
    submit_recv(fd);
}

void RomeServer::deal_send(std::unique_ptr<RSEvent> event) {
    if (event->res <= 0) {
        ROME_LOG("Sending no progress or error, errno: {}", event->res);
        return;
    }
    size_t sent = event->res;
    event->offset += sent;
    event->remaining -= sent;
    // check if all data is sent
    if (event->remaining == 0) {
        on_sent(std::move(event));
        return;
    }
    // continue sending remainning data
    auto result = _backend.submit(std::move(event));
    if (result != rome::uring::SubmitResult::SUCCESS) {
        ROME_LOG("Fail to submit send io event, {}", static_cast<int>(result));
        return;
    }
}

void RomeServer::close_connection(int client_fd) {
    close(client_fd);
    _connections.erase(client_fd);
}

bool RomeServer::run() {
    // initialize network
    _listen_fd = socket(_bind_addr.ss_family, SOCK_STREAM, 0);
    if (_listen_fd == -1) {
        ROME_LOG("Fail to start service. socket() fail.");
        return false;
    }

    int ret = bind(_listen_fd, reinterpret_cast<sockaddr *>(&_bind_addr), _bind_addr_len);
    if (ret == -1) {
        ROME_LOG("Fail to start service. bind() fail.");
        return false;
    }

    ret = listen(_listen_fd, SOMAXCONN);
    if (ret == -1) {
        ROME_LOG("Fail to start service. listen() fail.");
        return false;
    }

    event_loop();

    return true;
}

void RomeServer::shutdown() {
    _shutdown = true;
}

rome::uring::SubmitResult RomeServer::send(
    std::shared_ptr<Connection> conn,
    const std::array<std::byte, MAX_BUFFER_SIZE> &data,
    size_t length
) {
    if (length > MAX_BUFFER_SIZE) {
        return rome::uring::SubmitResult::SIZE_OVERFLOW;
    }

    if (!conn) {
        return rome::uring::SubmitResult::NULL_EVENT;
    }

    auto event = std::make_unique<RSEvent>();

    event->type = IOType::SEND;
    event->conn = std::move(conn);
    event->offset = 0;
    event->remaining = length;
    // TODO: do not copy?
    std::copy_n(data.begin(), length, event->buffer.begin());

    return _backend.submit(std::move(event));
}

} // namespace rome::server