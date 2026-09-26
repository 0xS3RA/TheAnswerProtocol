#pragma once

#include <initializer_list>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace tap {

using LogFields = std::initializer_list<std::pair<std::string_view, std::string>>;

class Logger {
public:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void info(std::string_view event_name, LogFields fields = {});
    void warn(std::string_view event_name, LogFields fields = {});
    void error(std::string_view event_name, LogFields fields = {});

private:
    void write(std::string_view level, std::string_view event_name, LogFields fields);
    void run();

    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<std::string> entries_;
    std::thread worker_;
    bool stopping_ = false;
};

} // namespace tap
