#include <array>
#include <cstddef>

#include <sys/socket.h>

constexpr size_t MAX_BUFFER_SIZE = 65536;

struct Connection {
    int fd = -1;
    
    sockaddr_storage client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
};

enum class IOType {
    ACCEPT, RECV, SEND
};

struct IOEvent {
    IOType type;
    std::shared_ptr<Connection> conn;
    int res = 0;                            // the result of an IO event

    virtual ~IOEvent() = default;
};

// event for receive and send, which require a buffer
struct RSEvent : IOEvent {
    std::array<std::byte, MAX_BUFFER_SIZE> buffer{};
    size_t remaining = 0;
    size_t offset = 0;
};

struct AcceptEvent : IOEvent {
    int listen_fd;

    AcceptEvent() {
        type = IOType::ACCEPT;
    }
};