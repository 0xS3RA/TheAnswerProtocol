#include "cli.hpp"
#include "Runner.hpp"
#include "common/template.pb.h"

void game_loop(Socket server_socket)
{
    std::string client_response{};

    while (true) {

        game::WorldDelta change{};
        game::RcvStatus status{};
        auto server_response =
            server_socket.receive_message<game::WorldDelta>(game::RcvStatus & status);
        switch (server_response.delta_type_case()) {

        case game::WorldDelta::kPlayerConnected:
            handlePlayerConnected(server_response);
        case game::WorldDelta::kPlayerDisconnected:
            handlePlayerDisconnected(server_response);
        case game::WorldDelta::kWorldInitiation:
            handleWorldInit(server_response);
        case game::WorldDelta::kMessageReceived:
            handleMessageReceived(server_response);
        case game::WorldDelta::DELTA_TYPE_NOT_SET:

            break;
        }
    }
}

int main()
{
    Socket server_socket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (server_socket.get() < 0)
        return (perror("Socket creation error"), EXIT_FAILURE);

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1332);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(server_socket.get(), reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
        return (perror("connection error"), EXIT_FAILURE);

    game_loop(std::move(server_socket));
    return (0);
}
