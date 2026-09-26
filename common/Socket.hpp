#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

namespace tap {

enum class ReadStatus { Line, Closed, TooLong, Error };

class LineReader {
public:
    explicit LineReader(int descriptor, std::size_t maximum_line_size = 1024);

    ReadStatus read(std::string& line);

private:
    int descriptor_;
    std::size_t maximum_line_size_;
    std::string buffer_;
    bool discarding_ = false;
};

class MessageSink {
public:
    virtual ~MessageSink() = default;
    virtual bool enqueue(std::string message) = 0;
};

class OutboundChannel final : public MessageSink {
public:
    explicit OutboundChannel(int descriptor, std::size_t capacity = 256);
    ~OutboundChannel() override;

    OutboundChannel(const OutboundChannel&) = delete;
    OutboundChannel& operator=(const OutboundChannel&) = delete;

    void start();
    bool enqueue(std::string message) override;
    void stop(bool drain);
    [[nodiscard]] bool running() const;

private:
    void write_loop();

    int descriptor_;
    std::size_t capacity_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<std::string> messages_;
    std::thread writer_;
    std::atomic<bool> running_{false};
    bool stopping_ = false;
    bool drain_ = false;
};

[[nodiscard]] bool send_line(int descriptor, std::string_view line);
[[nodiscard]] int connect_tcp(std::string_view host, std::string_view port);

} // namespace tap
