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
        return ::send(fd_, data.data(), data.size(), 0);
    }

    ssize_t recv(char *buffer, size_t capacity) const {
        if (!is_valid())
            return -1;
        return ::recv(fd_, buffer, capacity, 0);
    }

    std::string *receive_line() {
        std::string *result = new std::string {};
        std::string client_response {};
        while (true) {
            client_response.resize(1024);
            ssize_t bytes_received =
                recv(client_response.data(), client_response.size() - 1);
            if (bytes_received <= 0)
                return NULL;
            std::cout << bytes_received << std::endl;
            if (client_response[client_response.size() - 1] != '\n') {
                std::cout << "a: "<< client_response << std::endl;
                result->append(client_response);
                continue;
            } else {
                std::cout << "b: "<< client_response << std::endl;
                result->append(client_response);
                return (result);
            }
        }
    }

};
