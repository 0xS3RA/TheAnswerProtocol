#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace tap {

struct Command {
    std::string name;
    std::vector<std::string> arguments;

    [[nodiscard]] std::string rest(std::size_t index) const;
};

[[nodiscard]] Command parse_command(std::string_view line);
[[nodiscard]] std::string ok(std::string_view data = {});
[[nodiscard]] std::string error(int code, std::string_view message);
[[nodiscard]] std::string event(std::string_view category, std::string_view data);
[[nodiscard]] std::string json_string(std::string_view value);
[[nodiscard]] std::string lower_copy(std::string_view value);
[[nodiscard]] bool iequals(std::string_view lhs, std::string_view rhs);
[[nodiscard]] bool is_safe_text(std::string_view value);

} // namespace tap
