#pragma once

#include <queue>
#include <thread>
#include <atomic>

#include "Command.hpp"
#include "Socket.hpp"
#include "Change.hpp"
#include "ThreadSafeQueue.hpp"


class Runner {
    private:
        ThreadSafeQueue<Command> incomingQueue;
        ThreadSafeQueue<Change> outgoingQueue;
        std::atomic<bool> running;
        Socket clientSocket;
        std::thread worker;

        void run() {
            int max_iteration{50};
            Command received;

            while (running) {

                int i {0};
                Change change_ {};
                while (outgoingQueue.try_pop(change_)) {
                    i++;
                    clientSocket.send_change(change_);
                    if (i >= max_iteration) break;
                }

                ssize_t bytes = clientSocket.receive_command(received);
                if (bytes > 0) {
                    incomingQueue.push(received);
                } else if (bytes == 0){
                    break;
                } else {
                    std::this_thread::sleep_for(std::chrono:milliseconds(10));
                }
            }
        };

    public:
        explicit Runner(Socket sock) : clientSocket(std::move(sock)) {}

        ~Runner() {
            stop();
        }

        void start() {
            running = true;
            worker = std::thread(&Runner::run, this);
        }

        void stop() {
            if (running){
                running = false;
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }

        void send_change(const Change change) {
            outgoingQueue.push(change);
        }

        bool poll_incoming(Command &command){
            return incomingQueue.try_pop(command);
        }

           
}
