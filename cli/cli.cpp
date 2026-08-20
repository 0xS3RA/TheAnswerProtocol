#include "cli.hpp"

void game_loop(Socket &server_socket) {
    std::string client_response {};

    while (true) {
        auto server_response = server_socket.receive_line();
        if (server_response == "") return;
        if (server_response == "endsig") break;

        std::cout << server_response << std::endl;

        std::getline(std::cin, client_response);
        server_socket.send(client_response);
    }
}


int main() {
    Socket server_socket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (server_socket.get() < 0)
        return (perror("Socket creation error"), EXIT_FAILURE);

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1332);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(server_socket.get(), reinterpret_cast<struct sockaddr *>(&addr),
                sizeof(addr)) < 0)
        return (perror("connection error"), EXIT_FAILURE);

    game_loop(server_socket);

    return (0);
}
