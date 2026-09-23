#pragma once

#include <atomic>
#include <chrono>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <thread>

#include "common/Socket.hpp"
#include "common/ThreadSafeQueue.hpp"
#include "template.pb.h"

class Runner {
private:
    ThreadSafeQueue<game::CommandDelta> incomingQueue;
    ThreadSafeQueue<game::WorldDelta> outgoingQueue;
    uint64_t player_id{};
    std::string player_name{};
    std::atomic<bool> running;
    Socket clientSocket;
    std::thread worker;
    int event_fd{};

    void run()
    {
        int max_iteration{50};

        int epoll_fd = epoll_create1(0);

        epoll_event socket_event{};
        socket_event.events = EPOLLIN;
        socket_event.data.fd = clientSocket.get();
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientSocket.get(), &socket_event);

        epoll_event eventfd_event{};
        eventfd_event.events = EPOLLIN;
        eventfd_event.data.fd = event_fd;

        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &eventfd_event);

        while (running) {

            int i{0};
            game::WorldDelta change_{};
            epoll_event events[64];

            int count = epoll_wait(epoll_fd, events, 64, -1);

            for (auto j{0}; j < count; j++) {

                int fd = events[j].data.fd;
                if (fd == event_fd) {

                    uint64_t value{};
                    read(event_fd, &value, sizeof(value));

                    while (outgoingQueue.try_pop(change_)) {
                        i++;
                        clientSocket.send(change_);
                        if (change_.has_world_initiation())
                            std::cout << "sent world init for real" << std::endl;
                        if (i >= max_iteration)
                            break;
                    }
                }
                else if (fd == clientSocket.get()) {
                    game::RcvStatus status{};
                    auto command = clientSocket.receive_message<game::CommandDelta>(status);
                    if (status == game::RcvStatus::OK) {

                        std::cout << "Received message !" << std::endl;
                        // command is std::optional
                        // -> std::optional is the container, it has to be either dereferenced
                        // (*command) or .value() to get the object inside
                        // -> The incomingQueue takes a commandVariant in the Queue, not a
                        // std::optional.
                        incomingQueue.push(std::move(command.value()));
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
    explicit Runner(uint64_t playerId, std::string playerName, Socket sock)
        : player_id(playerId), player_name(playerName), clientSocket(std::move(sock)),
          event_fd(eventfd(0, 0))
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

    std::string_view get_player_name()
    {
        return player_name;
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

    void send_world_init(const game::World& world, game::Player& player)
    {
        game::WorldDelta worldInit{};
        auto* delta = worldInit.mutable_world_initiation();

        delta->set_start_location_id(world.start_location_id());
        for (const auto& item : world.items()) {
            *delta->add_items() = item;
        }
        for (const auto& npc : world.npcs()) {
            *delta->add_npcs() = npc;
        }
        for (const auto& location : world.locations()) {
            *delta->add_locations() = location;
        }
        for (const auto& player : world.players()) {
            (*delta->mutable_players())[player.id()] = player.name();
            (*delta->mutable_players_hp())[player.id()] = player.hp();
            (*delta->mutable_players_states())[player.id()] = player.state();
        }
        for (const auto& [player_id, location_id] : world.players_locations()) {
            (*delta->mutable_players_locations())[player_id] = location_id;
        }
        *delta->mutable_you() = player;
        outgoingQueue.push(worldInit);
        std::cout << "world init outgoing queue" << std::endl;
    }

    bool poll_incoming(game::CommandDelta& command)
    {
        return incomingQueue.try_pop(command);
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
