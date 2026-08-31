#pragma once

#include "iouring_backend.hpp"

namespace rome::server {

constexpr uint16_t DEFAULT_PORT = 8000;

class RomeServer {

private:
    sockaddr_storage _bind_addr{};
    socklen_t _bind_addr_len = 0;
    uint16_t _port = DEFAULT_PORT;
    int _listen_fd = 0;
    bool _shutdown = false;

    rome::uring::Backend _backend;

    std::unordered_map<int, std::shared_ptr<Connection>> _connections;

    rome::uring::SubmitResult submit_accept();
    rome::uring::SubmitResult submit_recv(int fd);
    void event_loop();

    void deal_accept(std::unique_ptr<AcceptEvent> event);
    void deal_recv(std::unique_ptr<RSEvent> event);
    void deal_send(std::unique_ptr<RSEvent> event);

    void close_connection(int client_fd);

public:
    RomeServer() {}
    RomeServer(const std::string &bind_addr, uint16_t port);
    virtual ~RomeServer() = default;

    bool run();
    void shutdown();

protected:
    virtual void on_start() {}
    virtual void on_recv(std::unique_ptr<RSEvent> event) = 0;
    virtual void on_sent(std::unique_ptr<RSEvent> event) {}
    virtual void on_disconnect(std::unique_ptr<RSEvent> event) {}
    virtual void on_shutdown() {}

    rome::uring::SubmitResult send(
        std::shared_ptr<Connection> conn,
        const std::array<std::byte, MAX_BUFFER_SIZE> &data,
        size_t length
    );

};

} // namespace rome::server