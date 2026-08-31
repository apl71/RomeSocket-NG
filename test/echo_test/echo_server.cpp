#include <iostream>
#include "rome_server.hpp"

class EchoServer : public rome::server::RomeServer {

public:
    EchoServer(const std::string &bind_addr, uint16_t port)
        : RomeServer(bind_addr, port) {}

protected:
    virtual void on_recv(std::unique_ptr<RSEvent> event) override {
        const size_t received = event->res;
        send(event->conn, event->buffer, received);
    }

};

int main() {
    EchoServer echo_server("0.0.0.0", 9898);
    echo_server.run();
}