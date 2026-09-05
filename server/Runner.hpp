#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "../common/Socket.hpp"
#include "../common/ThreadSafeQueue.hpp"
#include "../common/template.pb.h"

class Runner {
private:
    ThreadSafeQueue<game::CommandDelta> incomingQueue;
    ThreadSafeQueue<game::WorldDelta> outgoingQueue;
    uint64_t player_id{};
    std::atomic<bool> running;
    Socket clientSocket;
    std::thread worker;

    void run()
    {
        int max_iteration{50};

        while (running) {

            int i{0};
            game::WorldDelta change_{};
            while (outgoingQueue.try_pop(change_)) {
                i++;
                clientSocket.send(change_);
                if (i >= max_iteration)
                    break;
            }

            game::RcvStatus status{};
            auto command = clientSocket.receive_message<game::CommandDelta>(status);
            if (status == game::RcvStatus::OK) {

                // command is std::optional
                // -> std::optional is the container, it has to be either dereferenced
                // (*command) or .value() to get the object inside
                // -> The incomingQueue takes a commandVariant in the Queue, not a
                // std::optional.
                incomingQueue.push(std::move(command.value()));
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
    explicit Runner(uint64_t playerId, Socket sock)
        : player_id(playerId), clientSocket(std::move(sock))
    {
    }

    ~Runner()
    {
        stop();
    }

    uint64_t get_player_id()
    {
        return player_id;
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

    void send_change(const game::WorldDelta& change)
    {
        outgoingQueue.push(change);
    }

    bool poll_incoming(game::CommandDelta& command)
    {
        return incomingQueue.try_pop(command);
    }
};
