#pragma once

#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <iostream>
#include <variant>
#include <optional>

#include "template.pb.h"


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


    bool send(const google::protobuf::Message &msg) const {
        if (!is_valid())
            return false;
        std::string serialized_data;

        if (!msg.SerializeToString(&serialized_data))
            return false;

        uint32_t data_size = static_cast<uint32_t>(serialized_data.size()); // Envoyer la taille du message avant le message au lieu de mettre un '\n'

        ssize_t sent_size = ::send(fd_, &data_size, sizeof(data_size), 0);
        if (sent_size != sizeof(data_size))
            return false;

        ssize_t sent_data = ::send(fd_, serialized_data.data(), serialized_data.size(), 0);
        return sent_data == static_cast<ssize_t>(serialized_data.size());
    }

    bool read_all(char *destination, size_t size) const {

        size_t total_read = 0;
        while (total_read < size) {
            ssize_t bytes_read =
              ::recv(fd_, destination + total_read, size - total_read, 0);
            if (bytes_read <= 0)
                return false;
            total_read += bytes_read;
        }
        return true;
    }

    template <typename T>
    std::optional<T> receive_message(game::RcvStatus &status) {
      if (!is_valid()) {
          status = game::RcvStatus::DISCONNECTED;
          return std::nullopt;
      }
      uint32_t data_size = 0;
      if (!read_all(reinterpret_cast<char *>(&data_size), sizeof(data_size))) {
          status = game::RcvStatus::DISCONNECTED;
          return std::nullopt;
      }

      std::string buffer;
      buffer.resize(data_size);

      if (!read_all(&buffer[0], data_size)) {
          status = game::RcvStatus::DISCONNECTED;
          return std::nullopt;
      }

      T msg;
      if (!msg.ParseFromString(buffer)) {
          status = game::RcvStatus::PARSE_ERROR;
          return std::nullopt;
      }
      status = game::RcvStatus::OK;
      return msg;
    }
};
