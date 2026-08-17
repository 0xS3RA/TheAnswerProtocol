#include "server.hpp"

void client_loop(Socket &client_socket) {
    std::cout << client_socket.get() << std::endl;
}
