#include "rt/network/Socket.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace rt {
Socket::Socket(int handle) : handle_(handle) {
    if (handle_ < 0)
        throw std::runtime_error(std::string("socket: ") + std::strerror(errno));
    configure();
}

Socket::Socket(Socket &&other) noexcept : handle_(std::exchange(other.handle_, -1)) {}

Socket::~Socket() {
    if (handle_ >= 0)
        ::close(handle_);
}

void Socket::configure() {
    timeval timeout{30, 0};
    setsockopt(handle_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(handle_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#ifdef SO_NOSIGPIPE
    int yes = 1;
    setsockopt(handle_, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes));
#endif
}

Socket Socket::connect(const std::string &host, unsigned short port) {
    sockaddr_in endpoint{};
    endpoint.sin_family = AF_INET;
    endpoint.sin_port = htons(port);
    std::string numeric = host == "localhost" ? "127.0.0.1" : host;
    if (inet_pton(AF_INET, numeric.c_str(), &endpoint.sin_addr) != 1)
        throw std::runtime_error(
            "cluster endpoints require numeric IPv4 or localhost (no blocking DNS)");
    Socket connection(::socket(AF_INET, SOCK_STREAM, 0));
    int flags = fcntl(connection.handle_, F_GETFL, 0);
    if (flags < 0 || fcntl(connection.handle_, F_SETFL, flags | O_NONBLOCK) < 0)
        throw std::runtime_error("cannot configure nonblocking connect");
    int status =
        ::connect(connection.handle_, reinterpret_cast<sockaddr *>(&endpoint), sizeof(endpoint));
    if (status < 0 && errno != EINPROGRESS)
        throw std::runtime_error(std::string("connect: ") + std::strerror(errno));
    if (status < 0) {
        pollfd descriptor{connection.handle_, POLLOUT, 0};
        int ready;
        do {
            ready = ::poll(&descriptor, 1, 3000);
        } while (ready < 0 && errno == EINTR);
        int error = 0;
        socklen_t length = sizeof(error);
        if (ready <= 0 ||
            getsockopt(connection.handle_, SOL_SOCKET, SO_ERROR, &error, &length) != 0 || error)
            throw std::runtime_error("cluster connect failed or exceeded 3-second deadline");
    }
    if (fcntl(connection.handle_, F_SETFL, flags) < 0)
        throw std::runtime_error("cannot restore blocking socket");
    return connection;
}

Socket Socket::listen(unsigned short port, const std::string &address) {
    Socket listener(::socket(AF_INET, SOCK_STREAM, 0));
    timeval noTimeout{0, 0};
    setsockopt(listener.handle_, SOL_SOCKET, SO_RCVTIMEO, &noTimeout, sizeof(noTimeout));
    int yes = 1;
    setsockopt(listener.handle_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    sockaddr_in endpoint{};
    endpoint.sin_family = AF_INET;
    endpoint.sin_port = htons(port);
    if (inet_pton(AF_INET, address.c_str(), &endpoint.sin_addr) != 1)
        throw std::runtime_error("worker bind address must be IPv4");
    if (::bind(listener.handle_, reinterpret_cast<sockaddr *>(&endpoint), sizeof(endpoint)) != 0 ||
        ::listen(listener.handle_, 16) != 0)
        throw std::runtime_error(std::string("worker bind/listen: ") + std::strerror(errno));
    return listener;
}

Socket Socket::accept() const {
    timeval noTimeout{0, 0};
    setsockopt(handle_, SOL_SOCKET, SO_RCVTIMEO, &noTimeout, sizeof(noTimeout));
    int accepted;
    do {
        accepted = ::accept(handle_, nullptr, nullptr);
    } while (accepted < 0 && errno == EINTR);
    return Socket(accepted);
}

void Socket::send(const void *buffer, std::size_t count) const {
    const auto *data = static_cast<const char *>(buffer);
    std::size_t done = 0;
    while (done < count) {
        int flags = 0;
#ifdef MSG_NOSIGNAL
        flags = MSG_NOSIGNAL;
#endif
        ssize_t sent =
            ::send(handle_, data + done, std::min(count - done, std::size_t(1 << 20)), flags);
        if (sent < 0 && errno == EINTR)
            continue;
        if (sent <= 0)
            throw std::runtime_error("network send failed/disconnected");
        done += static_cast<std::size_t>(sent);
    }
}

void Socket::receive(void *buffer, std::size_t count) const {
    auto *data = static_cast<char *>(buffer);
    std::size_t done = 0;
    while (done < count) {
        ssize_t received =
            ::recv(handle_, data + done, std::min(count - done, std::size_t(1 << 20)), 0);
        if (received < 0 && errno == EINTR)
            continue;
        if (received <= 0)
            throw std::runtime_error("network receive failed, timed out or disconnected");
        done += static_cast<std::size_t>(received);
    }
}

void Socket::sendFrame(const std::string &payload) const {
    if (payload.size() > 1048576)
        throw std::runtime_error("network frame exceeds 1 MiB");
    std::uint32_t size = htonl(static_cast<std::uint32_t>(payload.size()));
    send(&size, sizeof(size));
    send(payload.data(), payload.size());
}

std::string Socket::receiveFrame(std::size_t limit) const {
    std::uint32_t size;
    receive(&size, sizeof(size));
    std::size_t count = ntohl(size);
    if (count > limit)
        throw std::runtime_error("network frame exceeds configured limit");
    std::string payload(count, '\0');
    receive(payload.data(), count);
    return payload;
}
} // namespace rt
