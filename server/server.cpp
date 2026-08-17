#include "server.hpp"

// 1 : Écris un petit serveur TCP monothread en C++ avec les fonctions POSIX
// (sys/socket.h) qui accepte un client, lit une ligne, réponds et se ferme.

// 2 : Ajoute une boucle while(true) et std::thread pour gérer plusieurs clients
// en même temps.

// 3 : Ajoute le parsing des commandes :help et :settings.

// 4 : Réécris le serveur avec Asio pour découvrir le réseau asynchrone moderne.


int main() {
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1337);
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

    while (true) {
        socklen_t socklen = sizeof(addr);
        Socket client_socket(accept(server_socket.get(), reinterpret_cast<struct sockaddr *>(&addr), &socklen));
        if (client_socket.get() < 0)
            return (perror("Accept error"), EXIT_FAILURE);
        client_loop(client_socket);
    }
}
