#pragma once

#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <iostream>


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
        if (!is_valid())
            return -1;
        std::string modified_data {data};
        modified_data += "\n";
        return ::send(fd_, modified_data.data(), modified_data.size(), 0);
    }

    ssize_t recv(char *buffer, size_t capacity) const {
        if (!is_valid())
            return -1;
        return ::recv(fd_, buffer, capacity, 0);
    }

    std::string receive_line() {
        std::string result = {};
        char buffer[1024];

        while (true) {
            std::cout << "Waiting to receive data..." << std::endl;
            ssize_t bytes_received =
                recv(buffer, sizeof(buffer));
            if (bytes_received <= 0) {
                std::cout << "Client disconnected : " << get() << std::endl;
                return "";
            }
            result.append(buffer, bytes_received);
            if (result.find('\n') != std::string::npos){
                return result;
            }
        }
    }

};
