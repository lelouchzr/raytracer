#pragma once
#include <cstddef>
#include <string>

namespace rt {
class Socket {
    int handle_;
    void configure();

  public:
    explicit Socket(int);
    ~Socket();
    Socket(Socket &&) noexcept;
    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;
    static Socket connect(const std::string &host, unsigned short port);
    static Socket listen(unsigned short port, const std::string &address = "127.0.0.1");
    Socket accept() const;
    void send(const void *, std::size_t) const;
    void receive(void *, std::size_t) const;
    void sendFrame(const std::string &) const;
    std::string receiveFrame(std::size_t limit = 1048576) const;
};
} // namespace rt
