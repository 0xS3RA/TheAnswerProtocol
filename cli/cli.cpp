#include "cli.hpp"
#include "../common/template.pb.h"
#include "Runner.hpp"
#include <ios>
#include <limits>
#include <notcurses/notcurses.h>
#include <string>

void game_loop(Socket server_socket, std::string_view name)
{

    game::ClientWorld localWorld{};
    game::Player myself{};
    myself.set_name(name);
    Runner runner = Runner(std::move(server_socket));
    runner.start();

    while (true) {

        std::string client_response{};

        game::WorldDelta change{};

        while (runner.poll_incoming(change)) {
            switch (change.delta_type_case()) {

            case game::WorldDelta::kWorldInitiation:
                handleWorldInit(change, localWorld);
                std::cout << "Received world initiation !!" << std::endl;
                break;
            case game::WorldDelta::kUnauthorizedCommand:
                handleUnauthorizedCommand(change);
                break;
            case game::WorldDelta::kPlayerConnected:
                handlePlayerConnected(change, localWorld);
                break;
            case game::WorldDelta::kPlayerDisconnected:
                handlePlayerDisconnected(change, localWorld);
                break;
            case game::WorldDelta::kMessageReceived:
                handleMessageReceived(change, localWorld);
                break;
            case game::WorldDelta::kPlayerInventoryChanged:
                handlePlayerInventoryChanged(change, localWorld);
                break;
            case game::WorldDelta::kNpcInventoryChanged:
                handleNpcInventoryChanged(change, localWorld);
                break;
            case game::WorldDelta::kPlayerHpChanged:
                handlePlayerHpChanged(change, localWorld);
                break;
            case game::WorldDelta::kNpcHpChanged:
                handleNpcHpChanged(change, localWorld);
                break;
            case game::WorldDelta::kPlayerMoved:
                handlePlayerMoved(change, localWorld);
                break;
            case game::WorldDelta::kDoorUnlock:
                handleDoorUnlock(change, localWorld);
                break;
            case game::WorldDelta::kPlayerStateChanged:
                handlePlayerStateChanged(change, localWorld);
                break;
            case game::WorldDelta::DELTA_TYPE_NOT_SET:
                break;
            }
        }
        std::cin.clear();
        std::cout << "ENTER COMMAND : " << std::endl;
        std::string command_{};
        while (!std::getline(std::cin, command_)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }

        if (localWorld.myself().state() == game::PlayerState::CHILLING) {
            if (command_ == ":INFO") {
                // Show current location's name
                // show current location's exits
                //
                continue;
            }
            else if (command_ == ":MESSAGE") {
                std::string message{};
                std::cout << "ENTER MESSAGE (OR :CANCEL TO CANCEL): ";
                while (!std::getline(std::cin, message)) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                }
                if (message == ":CANCEL")
                    continue;
                else {
                    game::CommandDelta message_command{};
                    auto* delta = message_command.mutable_message_command();
                    delta->set_message(message);
                    runner.send_command(message_command);
                    runner.notify_new();
                    std::cout << "Sent message : " << message << std::endl;
                }
            }
            else if (command_ == ":MOVE") {
                auto my_location_id = localWorld.players_locations().find(localWorld.myself().id());
                if (my_location_id == localWorld.players_locations().end()) {
                    std::cout << "It appears there was a problem, your player cannot be found on "
                                 "the map..."
                              << std::endl;
                    return;
                }
                auto location_object =
                    std::find_if(localWorld.locations().begin(), localWorld.locations().end(),
                                 [id = my_location_id->second](const game::Location& location) {
                                     return location.id() == id;
                                 });
                if (location_object == localWorld.locations().end()) {
                    std::cout << "It appears there was a problem, the location your player is in "
                                 "is not found anywhere..."
                              << std::endl;
                    return;
                }
                std::cout << "Choose which direction you wanna go (enter a number) :" << std::endl;
                int i{1};
                std::map<int, uint64_t> possible_directions{};
                std::cout << "0: cancel choice" << std::endl;
                for (const game::Exit& exit : location_object->exits()) {
                    if (exit.is_locked() == false) {

                        std::string direction_str{""};
                        std::string direction_name_str{""};

                        auto it = std::find_if(
                            localWorld.locations().begin(), localWorld.locations().end(),
                            [id = exit.location_id()](const game::Location& location) {
                                return id == location.id();
                            });
                        if (it == localWorld.locations().end()) {
                            std::cout << "This exit has a non-existant location id.." << std::endl;
                            return;
                        }
                        direction_name_str = it->name();

                        if (exit.direction() == game::Direction::NORTH)
                            direction_str += "North";
                        else if (exit.direction() == game::Direction::EAST)
                            direction_str += "East";
                        else if (exit.direction() == game::Direction::WEST)
                            direction_str += "West";
                        else if (exit.direction() == game::Direction::SOUTH)
                            direction_str += "South";

                        possible_directions[i] = exit.location_id();

                        std::cout << i << ": " << direction_str << " to " << direction_name_str
                                  << std::endl;
                        i++;
                    }
                }
                int user_response{};
                while (!(std::cin >> user_response) || user_response < 0 || user_response >= i) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cout << "Invalid choice, try again" << std::endl;
                }
                auto it = possible_directions.find(user_response);
                if (it == possible_directions.end()) {
                    std::cout << "you weirdly managed to enter a number that is not present in the "
                                 "possible directions, weird"
                              << std::endl;
                    return;
                }
                game::CommandDelta move_command{};
                auto* delta = move_command.mutable_interaction_command();
                delta->set_type(game::InteractionType::MOVE);
                delta->set_target_id(it->second);
                runner.send_command(move_command);
                std::cout << "Sent movement command" << std::endl;
            }
            else if (command_ == ":PICK_UP") {
            }
            else if (command_ == ":DROP") {
            }
            else if (command_ == ":OPEN") {
            }
            else if (command_ == ":SPEAK") {
            }
            else if (command_ == ":ATTACK") {
            }
            else if (command_ == ":USE") {
            }
            else if (command_ == ":SELL") {
            }
            else if (command_ == "GIVE") {
            }
        }
        else if (localWorld.myself().state() == game::PlayerState::IN_COMBAT) {
        }
        else {
            std::cout << "bro ur ded, stop trying" << std::endl;
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
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (connect(server_socket.get(), reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
        return (perror("connection error"), EXIT_FAILURE);

    game::CommandDelta command_{};
    auto* say_my_name = command_.mutable_name_set_command();
    say_my_name->set_name(name);
    server_socket.send(command_);

    game_loop(std::move(server_socket), name);
    return (0);
}
