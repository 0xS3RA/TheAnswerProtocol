#include "cli.hpp"
#include "../common/template.pb.h"
#include "Runner.hpp"
#include <notcurses/notcurses.h>

void game_loop(Socket server_socket, std::string_view name)
{

    game::ClientWorld localWorld{};
    game::Player myself{};
    myself.set_name(name);
    Runner runner = Runner(std::move(server_socket));

    while (true) {

        std::string client_response{};

        game::WorldDelta change{};

        while (runner.poll_incoming(change)) {
            if (game::RcvStatus::OK) {
                switch (change.delta_type_case()) {

                case game::WorldDelta::kWorldInitiation:
                    handleWorldInit(change, localWorld);
                case game::WorldDelta::kUnauthorizedCommand:
                    handleUnauthorizedCommand(change);
                case game::WorldDelta::kPlayerConnected:
                    handlePlayerConnected(change, localWorld);
                case game::WorldDelta::kPlayerDisconnected:
                    handlePlayerDisconnected(change, localWorld);
                case game::WorldDelta::kMessageReceived:
                    handleMessageReceived(change, localWorld);
                case game::WorldDelta::kPlayerInventoryChanged:
                    handlePlayerInventoryChanged(change, localWorld);
                case game::WorldDelta::kNpcInventoryChanged:
                    handleNpcInventoryChanged(change, localWorld);
                case game::WorldDelta::kPlayerHpChanged:
                    handlePlayerHpChanged(change, localWorld);
                case game::WorldDelta::kNpcHpChanged:
                    handleNpcHpChanged(change, localWorld);
                case game::WorldDelta::kPlayerMoved:
                    handlePlayerMoved(change, localWorld);
                case game::WorldDelta::kNpcMoved:
                    hanleNpcMoved(change stdplane);
                case game::WorldDelta::kDoorUnlock:
                    handleDoorUnlock(change stdplane);
                case game::WorldDelta::kPlayerStateChanged:
                    handlePlayerStateChanged(change, localWorld);
                case game::WorldDelta::DELTA_TYPE_NOT_SET:
                    break;
                }
            }
            else {
                break;
            }
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

    std::string name{};
    std::cout << "Choose a name to connect : ";
    std::cin >> std::ws >> name;

    if (connect(server_socket.get(), reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
        return (perror("connection error"), EXIT_FAILURE);

    game::CommandDelta command_{};
    auto* say_my_name = command_.mutable_name_set_command();
    say_my_name->set_name(name);
    server_socket.send(command_);

    game_loop(std::move(server_socket), name);
    return (0);
}
