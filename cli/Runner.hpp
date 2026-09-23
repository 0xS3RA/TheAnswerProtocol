
#pragma once

#include <atomic>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <thread>

#include "common/Socket.hpp"
#include "common/ThreadSafeQueue.hpp"
#include "template.pb.h"

class Runner {
private:
    ThreadSafeQueue<game::WorldDelta> incomingQueue;
    ThreadSafeQueue<game::CommandDelta> outgoingQueue;
    std::atomic<bool> running;
    Socket serverSocket;
    std::thread worker;
    int event_fd{};

    void run()
    {
        int max_iteration{50};

        int epoll_fd = epoll_create1(0);

        epoll_event socket_event{};
        socket_event.events = EPOLLIN;
        socket_event.data.fd = serverSocket.get();
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket.get(), &socket_event);

        epoll_event eventfd_event{};
        eventfd_event.events = EPOLLIN;
        eventfd_event.data.fd = event_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &eventfd_event);

        while (running) {

            int i{0};
            game::CommandDelta command_{};
            epoll_event events[64];

            int count = epoll_wait(epoll_fd, events, 64, -1);

            for (int j{0}; j < count; j++) {

                int fd = events[j].data.fd;
                if (fd == event_fd) {

                    uint64_t value{};
                    read(event_fd, &value, sizeof(value));

                    while (outgoingQueue.try_pop(command_)) {
                        i++;
                        serverSocket.send(command_);
                        if (command_.has_message_command())
                            std::cout << "sent message for real" << std::endl;
                        if (i >= max_iteration)
                            break;
                    }
                }
                else if (fd == serverSocket.get()) {
                    game::RcvStatus status{};
                    auto change = serverSocket.receive_message<game::WorldDelta>(status);
                    if (status == game::RcvStatus::OK) {

                        std::cout << "Received message !!" << std::endl;
                        // command is std::optional
                        // -> std::optional is the container, it has to be either dereferenced
                        // (*command) or .value() to get the object inside
                        // -> The incomingQueue takes a commandVariant in the Queue, not a
                        // std::optional.
                        incomingQueue.push(std::move(change.value()));
                    }
                    else if (status == game::RcvStatus::DISCONNECTED) {
                        running = false;
                        break;
                    }
                    else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                }
            }
        }
    };

public:
    explicit Runner(Socket sock) : serverSocket(std::move(sock)), event_fd(eventfd(0, 0)) {}

    ~Runner()
    {
        stop();
    }

    bool is_running()
    {
        return running;
    }

    void start()
    {
        running = true;
        worker = std::thread(&Runner::run, this);
    }

    void stop()
    {
        if (running) {
            running = false;
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void send_command(const game::CommandDelta& command)
    {
        outgoingQueue.push(command);
    }

    bool poll_incoming(game::WorldDelta& change)
    {
        return incomingQueue.try_pop(change);
    }

    bool notify_new()
    {
        uint64_t value{1};
        auto bytes = write(event_fd, &value, sizeof(value));
        if (bytes == -1)
            return false;
        return true;
    }
};
