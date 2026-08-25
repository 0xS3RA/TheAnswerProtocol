#include "server.hpp"
#include <cstdlib>
#include <netinet/in.h>
#include <thread>
#include <vector>


int accept_loop(struct sockaddr_in addr, Socket &server_socket) {
    std::vector<std::jthread> thread_vector {};

    while (true) {
        socklen_t socklen = sizeof(addr);
        Socket client_socket(accept(server_socket.get(), reinterpret_cast<struct sockaddr *>(&addr), &socklen));
        if (client_socket.get() < 0)
            return (perror("Accept error"), EXIT_FAILURE);
        thread_vector.emplace_back(client_loop, std::move(client_socket));
    }
}

int main() {
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1332);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);


    Socket server_socket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (server_socket.get() < 0)
        return (perror("Socket creation error"), EXIT_FAILURE);

    if (bind(server_socket.get(), reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1) {
        perror("Bind error");
        return EXIT_FAILURE;
    }

    if (listen(server_socket.get(), 10) == -1) {
        perror("Listen error");
        return EXIT_FAILURE;
    }

    return (accept_loop(addr, server_socket) == EXIT_FAILURE);
}
