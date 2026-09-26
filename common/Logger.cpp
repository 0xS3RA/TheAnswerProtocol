#include "common/Logger.hpp"

#include "common/Protocol.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace tap {
namespace {

std::string timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t value = std::chrono::system_clock::to_time_t(now);
    std::tm time{};
    gmtime_r(&value, &time);

    std::ostringstream output;
    output << std::put_time(&time, "%Y-%m-%dT%H:%M:%S") << '.' << std::setw(3)
           << std::setfill('0') << milliseconds.count() << 'Z';
    return output.str();
}

} // namespace

Logger::Logger() : worker_(&Logger::run, this) {}

Logger::~Logger()
{
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
    }
    condition_.notify_one();
    worker_.join();
}

void Logger::info(std::string_view event_name, LogFields fields)
{
    write("INFO", event_name, fields);
}

void Logger::warn(std::string_view event_name, LogFields fields)
{
    write("WARN", event_name, fields);
}

void Logger::error(std::string_view event_name, LogFields fields)
{
    write("ERROR", event_name, fields);
}

void Logger::write(std::string_view level, std::string_view event_name, LogFields fields)
{
    std::ostringstream line;
    line << "{\"ts\":" << json_string(timestamp()) << ",\"level\":" << json_string(level)
         << ",\"event\":" << json_string(event_name);
    for (const auto& [key, value] : fields)
        line << ',' << json_string(key) << ':' << json_string(value);
    line << '}';

    {
        std::lock_guard lock(mutex_);
        entries_.push_back(line.str());
    }
    condition_.notify_one();
}

void Logger::run()
{
    while (true) {
        std::string entry;
        {
            std::unique_lock lock(mutex_);
            condition_.wait(lock, [this] { return stopping_ || !entries_.empty(); });
            if (entries_.empty() && stopping_)
                break;
            entry = std::move(entries_.front());
            entries_.pop_front();
        }
        std::cerr << entry << '\n';
    }
}

} // namespace tap
