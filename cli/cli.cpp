#include "cli.hpp"

void game_loop(Socket &server_socket) {
    char server_response[1024];
    std::string client_response {};
    memset(server_response, 0, 1024);

    while (true) {
        memset(server_response, 0, 1024);
        int bytes_received = recv(server_socket.get(), server_response, 1023, 0);
        if (bytes_received <= 0) break;
        if (strcmp(server_response, "endsig") == 0)
            break;

        std::cout << server_response << std::endl;

        std::getline(std::cin, client_response);
        send(server_socket.get(), client_response.c_str(),
             client_response.length(), 0);
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
