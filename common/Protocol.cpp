#include "common/Protocol.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace tap {

std::string Command::rest(std::size_t index) const
{
    std::string result;
    for (std::size_t i = index; i < arguments.size(); ++i) {
        if (!result.empty())
            result += ' ';
        result += arguments[i];
    }
    return result;
}

Command parse_command(std::string_view line)
{
    Command command;
    std::istringstream stream{std::string(line)};
    stream >> command.name;
    std::transform(command.name.begin(), command.name.end(), command.name.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::toupper(character));
                   });

    std::string argument;
    while (stream >> argument)
        command.arguments.push_back(std::move(argument));
    return command;
}

std::string ok(std::string_view data)
{
    return data.empty() ? "OK" : "OK " + std::string(data);
}

std::string error(int code, std::string_view message)
{
    return "ERR " + std::to_string(code) + " " + std::string(message);
}

std::string event(std::string_view category, std::string_view data)
{
    return "EVT " + std::string(category) + " " + std::string(data);
}

std::string json_string(std::string_view value)
{
    std::ostringstream output;
    output << '"';
    for (const unsigned char character : value) {
        switch (character) {
        case '"':
            output << "\\\"";
            break;
        case '\\':
            output << "\\\\";
            break;
        case '\b':
            output << "\\b";
            break;
        case '\f':
            output << "\\f";
            break;
        case '\n':
            output << "\\n";
            break;
        case '\r':
            output << "\\r";
            break;
        case '\t':
            output << "\\t";
            break;
        default:
            if (character < 0x20)
                output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                       << static_cast<int>(character) << std::dec;
            else
                output << static_cast<char>(character);
        }
    }
    output << '"';
    return output.str();
}

std::string lower_copy(std::string_view value)
{
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return result;
}

bool iequals(std::string_view lhs, std::string_view rhs)
{
    return lower_copy(lhs) == lower_copy(rhs);
}

bool is_safe_text(std::string_view value)
{
    for (std::size_t index = 0; index < value.size();) {
        const auto first = static_cast<unsigned char>(value[index]);
        if (first < 0x20 || first == 0x7f)
            return false;
        if (first < 0x80) {
            ++index;
            continue;
        }

        std::size_t length = 0;
        std::uint32_t codepoint = 0;
        if (first >= 0xc2 && first <= 0xdf) {
            length = 2;
            codepoint = first & 0x1f;
        }
        else if (first >= 0xe0 && first <= 0xef) {
            length = 3;
            codepoint = first & 0x0f;
        }
        else if (first >= 0xf0 && first <= 0xf4) {
            length = 4;
            codepoint = first & 0x07;
        }
        else
            return false;
        if (index + length > value.size())
            return false;
        for (std::size_t offset = 1; offset < length; ++offset) {
            const auto continuation = static_cast<unsigned char>(value[index + offset]);
            if ((continuation & 0xc0) != 0x80)
                return false;
            codepoint = (codepoint << 6) | (continuation & 0x3f);
        }
        if ((length == 3 && codepoint < 0x800) || (length == 4 && codepoint < 0x10000) ||
            (codepoint >= 0xd800 && codepoint <= 0xdfff) || codepoint > 0x10ffff)
            return false;
        index += length;
    }
    return true;
}

} // namespace tap
