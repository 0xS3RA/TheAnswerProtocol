#pragma once

#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>


class Socket {
private:
    int fd_ = -1;

public:
    explicit Socket(int fd = -1) : fd_{fd} {}

    ~Socket() { close(); }

    Socket(const Socket &) = delete; // Constructeur de copie
    Socket &operator=(const Socket &) = delete; // opérateur de copie


    // Constructeur de déplacement
    Socket(Socket &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

    // Opérateur d'assignation par déplacement
    Socket &operator=(Socket &&other) noexcept {
        if (this != &other) {
            close();
            fd_ = std::exchange(other.fd_, -1);
      }
        return *this;
    }


    void close() {
        if (fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
      }
    }

    bool is_valid() const { return fd_ != -1; }
    int get() const { return fd_; }


    ssize_t send(std::string_view data) const {
        if (is_valid())
            return -1;
        return ::send(fd_, data.data(), data.size(), 0);
    }

    ssize_t recv(char *buffer, size_t capacity) const {
        if (!is_valid())
            return -1;
        return ::recv(fd_, buffer, capacity, 0);
    }

};
