#include "common/Logger.hpp"
#include "common/Protocol.hpp"
#include "common/Socket.hpp"
#include "server/Game.hpp"
#include "server/World.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {

struct Options {
    std::string host = "127.0.0.1";
    std::string port = "4242";
    std::string world = "world.yaml";
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
        else if (flag == "--world")
            options.world = argv[index + 1];
        else
            throw std::runtime_error("Unknown option: " + flag);
    }
    return options;
}

int create_listener(const Options& options)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* addresses = nullptr;
    const char* host = options.host.empty() ? nullptr : options.host.c_str();
    if (::getaddrinfo(host, options.port.c_str(), &hints, &addresses) != 0)
        return -1;

    int listener = -1;
    for (addrinfo* address = addresses; address != nullptr; address = address->ai_next) {
        listener = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (listener < 0)
            continue;
        int reuse = 1;
        ::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        if (::bind(listener, address->ai_addr, address->ai_addrlen) == 0 &&
            ::listen(listener, 64) == 0)
            break;
        ::close(listener);
        listener = -1;
    }
    ::freeaddrinfo(addresses);
    return listener;
}

std::string peer_address(const sockaddr_storage& address, socklen_t length)
{
    char host[NI_MAXHOST]{};
    char service[NI_MAXSERV]{};
    if (::getnameinfo(reinterpret_cast<const sockaddr*>(&address), length, host, sizeof(host),
                      service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV) != 0)
        return "unknown";
    return std::string(host) + ':' + service;
}

std::string peer_host(const sockaddr_storage& address, socklen_t length)
{
    char host[NI_MAXHOST]{};
    if (::getnameinfo(reinterpret_cast<const sockaddr*>(&address), length, host, sizeof(host),
                      nullptr, 0, NI_NUMERICHOST) != 0)
        return "unknown";
    return host;
}

void serve_client(int descriptor, std::string remote, std::shared_ptr<tap::Game> game,
                  std::shared_ptr<tap::Logger> logger)
{
    auto output = std::make_shared<tap::OutboundChannel>(descriptor);
    try {
        output->start();
    }
    catch (const std::exception& exception) {
        logger->error("writer_thread_failed",
                      {{"remote", remote}, {"reason", exception.what()}});
        ::close(descriptor);
        return;
    }
    const std::string greeting = tap::ok("hello proto=1");
    if (!output->enqueue(greeting)) {
        output->stop(false);
        ::close(descriptor);
        return;
    }
    logger->info("response", {{"remote", remote}, {"response", greeting}});
    tap::LineReader reader(descriptor);
    std::string player_name;
    bool graceful = false;
    auto window_start = std::chrono::steady_clock::now();
    unsigned commands = 0;

    while (output->running()) {
        std::string line;
        const tap::ReadStatus status = reader.read(line);
        if (status == tap::ReadStatus::TooLong) {
            output->enqueue(tap::error(400, "BAD_REQUEST"));
            logger->warn("line_too_long", {{"remote", remote}});
            continue;
        }
        if (status != tap::ReadStatus::Line)
            break;
        if (!tap::is_safe_text(line)) {
            const std::string reply = tap::error(400, "BAD_REQUEST");
            output->enqueue(reply);
            logger->warn("response_error", {{"remote", remote}, {"response", reply}});
            continue;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now - window_start >= std::chrono::seconds(1)) {
            window_start = now;
            commands = 0;
        }
        if (++commands == 21)
            logger->warn("abuse_command_flood", {{"remote", remote}, {"player", player_name}});

        const tap::Command command = tap::parse_command(line);
        logger->info("command_received",
                     {{"remote", remote}, {"player", player_name}, {"line", line}});
        if (player_name.empty()) {
            if (command.name != "CONNECT" || command.arguments.size() != 1) {
                const std::string reply = tap::error(400, "BAD_REQUEST");
                output->enqueue(reply);
                logger->warn("response_error", {{"remote", remote}, {"response", reply}});
                continue;
            }
            const std::string reply = game->connect(command.arguments.front(), output);
            if (!output->enqueue(reply))
                break;
            logger->info("response", {{"remote", remote}, {"response", reply}});
            if (!reply.starts_with("ERR"))
                player_name = command.arguments.front();
            continue;
        }

        const tap::CommandResult result = game->handle(player_name, command);
        if (!output->enqueue(result.reply))
            break;
        if (result.quit) {
            graceful = true;
            break;
        }
    }

    if (!player_name.empty())
        game->disconnect(player_name);
    output->stop(graceful);
    ::shutdown(descriptor, SHUT_RDWR);
    ::close(descriptor);
    logger->info("connection_close", {{"remote", remote}, {"player", player_name}});
}

} // namespace

int main(int argc, char** argv)
{
    std::signal(SIGPIPE, SIG_IGN);
    auto logger = std::make_shared<tap::Logger>();
    try {
        const Options options = parse_options(argc, argv);
        auto game = std::make_shared<tap::Game>(tap::World::load(options.world), *logger);
        const int listener = create_listener(options);
        if (listener < 0)
            throw std::runtime_error("Could not bind " + options.host + ':' + options.port);

        logger->info("server_started",
                     {{"host", options.host}, {"port", options.port}, {"world", options.world}});
        std::map<std::string, std::deque<std::chrono::steady_clock::time_point>> recent_connections;
        while (true) {
            sockaddr_storage address{};
            socklen_t length = sizeof(address);
            const int descriptor =
                ::accept(listener, reinterpret_cast<sockaddr*>(&address), &length);
            if (descriptor < 0) {
                if (errno == EINTR)
                    continue;
                logger->error("accept_failed", {{"reason", std::strerror(errno)}});
                continue;
            }
            timeval send_timeout{};
            send_timeout.tv_sec = 2;
            ::setsockopt(descriptor, SOL_SOCKET, SO_SNDTIMEO, &send_timeout,
                         sizeof(send_timeout));
            const std::string remote = peer_address(address, length);
            const std::string host = peer_host(address, length);
            auto& history = recent_connections[host];
            const auto now = std::chrono::steady_clock::now();
            while (!history.empty() && now - history.front() > std::chrono::seconds(10))
                history.pop_front();
            history.push_back(now);
            if (history.size() == 11)
                logger->warn("abuse_rapid_connections",
                             {{"host", host}, {"count", std::to_string(history.size())}});
            logger->info("connection_open", {{"remote", remote}});
            try {
                std::thread(serve_client, descriptor, remote, game, logger).detach();
            }
            catch (const std::exception& exception) {
                logger->error("connection_thread_failed",
                              {{"remote", remote}, {"reason", exception.what()}});
                ::close(descriptor);
            }
        }
    }
    catch (const std::exception& exception) {
        logger->error("server_fatal", {{"reason", exception.what()}});
        std::cerr << "tap-server: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }
}
