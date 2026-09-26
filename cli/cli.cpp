#include "common/Protocol.hpp"
#include "common/Socket.hpp"

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {

struct Options {
    std::string host = "127.0.0.1";
    std::string port = "4242";
    std::string name;
};

Options parse_options(int argc, char** argv)
{
    Options options;
    for (int index = 1; index < argc; index += 2) {
        if (index + 1 >= argc)
            throw std::runtime_error("Expected a value after " + std::string(argv[index]));
        const std::string flag = argv[index];
        if (flag == "--host")
            options.host = argv[index + 1];
        else if (flag == "--port")
            options.port = argv[index + 1];
        else if (flag == "--name")
            options.name = argv[index + 1];
        else
            throw std::runtime_error("Unknown option: " + flag);
    }
    return options;
}

} // namespace

int main(int argc, char** argv)
{
    try {
        const Options options = parse_options(argc, argv);
        const int descriptor = tap::connect_tcp(options.host, options.port);
        if (descriptor < 0)
            throw std::runtime_error("Could not connect to " + options.host + ':' + options.port);

        tap::LineReader greeting_reader(descriptor);
        std::string greeting;
        if (greeting_reader.read(greeting) != tap::ReadStatus::Line)
            throw std::runtime_error("Server closed before sending its greeting");
        std::cout << greeting << '\n';

        if (!options.name.empty() && !tap::send_line(descriptor, "CONNECT " + options.name))
            throw std::runtime_error("Could not send CONNECT");

        std::atomic<bool> connected = true;
        std::thread receiver([&] {
            tap::LineReader reader(descriptor);
            while (connected) {
                std::string line;
                const tap::ReadStatus status = reader.read(line);
                if (status != tap::ReadStatus::Line)
                    break;
                std::cout << line << '\n';
            }
            connected = false;
        });

        std::string line;
        while (connected) {
            pollfd input{STDIN_FILENO, POLLIN, 0};
            const int ready = ::poll(&input, 1, 100);
            if (ready < 0) {
                if (errno == EINTR)
                    continue;
                break;
            }
            if (ready == 0)
                continue;
            if ((input.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0 ||
                !std::getline(std::cin, line))
                break;
            if (line.size() > 1024) {
                std::cerr << "Command rejected locally: maximum length is 1024 bytes\n";
                continue;
            }
            if (!tap::is_safe_text(line)) {
                std::cerr << "Command rejected locally: control characters are not allowed\n";
                continue;
            }
            if (!tap::send_line(descriptor, line))
                break;
            if (tap::parse_command(line).name == "QUIT")
                break;
        }

        ::shutdown(descriptor, SHUT_WR);
        receiver.join();
        ::close(descriptor);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception) {
        std::cerr << "tap-cli: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }
}
