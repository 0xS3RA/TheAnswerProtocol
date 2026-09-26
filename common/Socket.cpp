#include "common/Socket.hpp"

#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

namespace tap {

LineReader::LineReader(int descriptor, std::size_t maximum_line_size)
    : descriptor_(descriptor), maximum_line_size_(maximum_line_size)
{
}

ReadStatus LineReader::read(std::string& line)
{
    while (true) {
        const std::size_t newline = buffer_.find('\n');
        if (discarding_) {
            if (newline != std::string::npos) {
                buffer_.erase(0, newline + 1);
                discarding_ = false;
                return ReadStatus::TooLong;
            }
        }
        if (newline != std::string::npos) {
            line = buffer_.substr(0, newline);
            buffer_.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            return line.size() <= maximum_line_size_ ? ReadStatus::Line : ReadStatus::TooLong;
        }
        if (buffer_.size() > maximum_line_size_) {
            buffer_.clear();
            discarding_ = true;
        }

        char chunk[512];
        const ssize_t count = ::recv(descriptor_, chunk, sizeof(chunk), 0);
        if (count == 0)
            return ReadStatus::Closed;
        if (count < 0) {
            if (errno == EINTR)
                continue;
            return ReadStatus::Error;
        }
        buffer_.append(chunk, static_cast<std::size_t>(count));
    }
}

OutboundChannel::OutboundChannel(int descriptor, std::size_t capacity)
    : descriptor_(descriptor), capacity_(capacity)
{
}

OutboundChannel::~OutboundChannel()
{
    stop(false);
}

void OutboundChannel::start()
{
    running_ = true;
    writer_ = std::thread(&OutboundChannel::write_loop, this);
}

bool OutboundChannel::enqueue(std::string message)
{
    std::lock_guard lock(mutex_);
    if (!running_ || stopping_)
        return false;
    if (messages_.size() >= capacity_) {
        stopping_ = true;
        drain_ = false;
        messages_.clear();
        running_ = false;
        ::shutdown(descriptor_, SHUT_RDWR);
        condition_.notify_all();
        return false;
    }
    messages_.push_back(std::move(message));
    condition_.notify_one();
    return true;
}

void OutboundChannel::stop(bool drain)
{
    {
        std::lock_guard lock(mutex_);
        if (!running_ && !writer_.joinable())
            return;
        stopping_ = true;
        drain_ = drain;
        if (!drain_)
            messages_.clear();
    }
    condition_.notify_all();
    if (!drain)
        ::shutdown(descriptor_, SHUT_RDWR);
    if (writer_.joinable())
        writer_.join();
    running_ = false;
}

bool OutboundChannel::running() const
{
    return running_;
}

void OutboundChannel::write_loop()
{
    while (true) {
        std::string message;
        {
            std::unique_lock lock(mutex_);
            condition_.wait(lock, [this] { return stopping_ || !messages_.empty(); });
            if (stopping_ && (!drain_ || messages_.empty()))
                break;
            message = std::move(messages_.front());
            messages_.pop_front();
        }
        if (!send_line(descriptor_, message)) {
            running_ = false;
            ::shutdown(descriptor_, SHUT_RDWR);
            break;
        }
    }
    running_ = false;
}

bool send_line(int descriptor, std::string_view line)
{
    std::string framed(line);
    framed.push_back('\n');
    std::size_t written = 0;
    while (written < framed.size()) {
        const ssize_t count =
            ::send(descriptor, framed.data() + written, framed.size() - written, MSG_NOSIGNAL);
        if (count < 0) {
            if (errno == EINTR)
                continue;
            return false;
        }
        written += static_cast<std::size_t>(count);
    }
    return true;
}

int connect_tcp(std::string_view host, std::string_view port)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* addresses = nullptr;
    if (::getaddrinfo(std::string(host).c_str(), std::string(port).c_str(), &hints, &addresses) != 0)
        return -1;

    int descriptor = -1;
    for (addrinfo* address = addresses; address != nullptr; address = address->ai_next) {
        descriptor = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (descriptor < 0)
            continue;
        if (::connect(descriptor, address->ai_addr, address->ai_addrlen) == 0)
            break;
        ::close(descriptor);
        descriptor = -1;
    }
    ::freeaddrinfo(addresses);
    return descriptor;
}

} // namespace tap
