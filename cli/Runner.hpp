
#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "../common/Socket.hpp"
#include "../common/ThreadSafeQueue.hpp"
#include "../common/template.pb.h"

class Runner {
private:
    ThreadSafeQueue<game::WorldDelta> incomingQueue;
    ThreadSafeQueue<game::CommandDelta> outgoingQueue;
    std::atomic<bool> running;
    Socket serverSocket;
    std::thread worker;

    void run()
    {
        int max_iteration{50};

        while (running) {

            int i{0};
            game::CommandDelta command_{};
            while (outgoingQueue.try_pop(command_)) {
                i++;
                serverSocket.send(command_);
                if (i >= max_iteration)
                    break;
            }

            game::RcvStatus status{};
            auto change = serverSocket.receive_message<game::WorldDelta>(status);
            if (status == game::RcvStatus::OK) {

                // command is std::optional
                // -> std::optional is the container, it has to be either dereferenced
                // (*command) or .value() to get the object inside
                // -> The incomingQueue takes a commandVariant in the Queue, not a
                // std::optional.
                incomingQueue.push(std::move(change.value()));
            }
            else if (status == game::RcvStatus::DISCONNECTED) {
                break;
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    };

public:
    explicit Runner(Socket sock) : serverSocket(std::move(sock)) {}

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

    void send_change(const game::CommandDelta& command)
    {
        outgoingQueue.push(command);
    }

    bool poll_incoming(game::WorldDelta& change)
    {
        return incomingQueue.try_pop(change);
    }
};
