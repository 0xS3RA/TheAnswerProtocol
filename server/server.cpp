#include "server.hpp"
#include <cstdlib>
#include <memory>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "../common/template.pb.h"
#include "Runner.hpp"
#include "common/Socket.hpp"
#include "common/ThreadSafeQueue.hpp"

namespace {
game::Direction parse_direction(const std::string& direction)
{
    if (direction == "north")
        return game::NORTH;
    if (direction == "east")
        return game::EAST;

    if (direction == "south")
        return game::SOUTH;

    if (direction == "west")
        return game::WEST;

    throw std::runtime_error("Unknown direction : " + direction);
}

game::Usage parse_usage(const std::string& usage)
{
    if (usage == "self")
        return game::SELF;

    if (usage == "other")
        return game::OTHER;

    if (usage == "hybrid")
        return game::HYBRID;

    if (usage == "world")
        return game::WORLD;

    throw std::runtime_error("Unknown item usage : " + usage);
}

game::Temperament parse_temperament(const std::string& temperament)
{
    if (temperament == "passive")
        return game::PASSIVE;

    if (temperament == "defensive")
        return game::DEFENSIVE;

    if (temperament == "aggressive")
        return game::AGGRESIVE;

    throw std::runtime_error("Unknown temperament : " + temperament);
}

game::EffectType parse_effect_type(const std::string& type)
{
    if (type == "heal")
        return game::HEAL;

    if (type == "damage")
        return game::DAMAGE;

    if (type == "energy_buff")
        return game::ENERGY_BUFF;

    if (type == "energy_debuff")
        return game::ENERGY_DEBUFF;

    if (type == "key")
        return game::KEY;

    throw std::runtime_error("Unknown effect type : " + type);
}

void parse_item_effect(const YAML::Node& effect_node, game::Item& item)
{
    const std::string effect = effect_node.as<std::string>();

    const std::size_t separator = effect.find(':');

    if (separator == std::string::npos) {
        throw std::runtime_error("Invalid effect" + effect);
    }

    const std::string type = effect.substr(0, separator);

    const std::string amount_string = effect.substr(separator + 1);

    const uint32_t amount = static_cast<uint32_t>(std::stoul(amount_string));

    game::Effect* proto_effect = item.add_effects();

    proto_effect->set_type(parse_effect_type(type));

    proto_effect->set_amount(amount);
}

void parse_npc_inventory_entry(const YAML::Node& inventory_node, game::Npc& npc,
                               const std::unordered_map<std::string, uint64_t>& item_ids)
{
    const std::string entry = inventory_node.as<std::string>();

    std::vector<std::string> parts;

    std::size_t start = 0;

    while (true) {
        const std::size_t separator = entry.find(':', start);

        if (separator == std::string::npos) {
            parts.push_back(entry.substr(start));
            break;
        }

        parts.push_back(entry.substr(start, separator - start));

        start = separator + 1;
    }

    if (parts.size() < 2 || parts.size() > 3) {
        throw std::runtime_error("Invalid inventory : " + entry);
    }

    const std::string& item_name = parts[0];

    if (!item_ids.contains(item_name)) {
        throw std::runtime_error("Unknown Item in Npc's inventory : " + item_name);
    }

    const uint64_t item_id = item_ids.at(item_name);

    const uint64_t quantity = std::stoull(parts[1]);

    npc.mutable_inventory()->operator[](item_id) = quantity;

    if (parts.size() == 3) {

        const uint64_t price = std::stoull(parts[2]);

        npc.mutable_trade_inventory()->operator[](item_id) = price;
    }
}
} // namespace

bool init_world(const std::string& file_path, game::World& world)
{
    std::cout << "World Initialization..." << std::endl;

    try {
        YAML::Node config = YAML::LoadFile(file_path);

        uint64_t next_item_id = 1;
        uint64_t next_npc_id = 1;
        uint64_t next_location_id = 1;

        std::unordered_map<std::string, uint64_t> item_ids;
        std::unordered_map<std::string, uint64_t> npc_ids;
        std::unordered_map<std::string, uint64_t> location_ids;

        if (config["items"]) {

            const YAML::Node items = config["items"];

            if (!items.IsMap()) {
                throw std::runtime_error("'items' needs to be a YAML map");
            }

            for (const auto& entry : items) {

                const std::string item_name = entry.first.as<std::string>();

                const YAML::Node item_node = entry.second;

                const uint64_t id = next_item_id++;

                item_ids[item_name] = id;

                game::Item* item = world.add_items();

                item->set_id(id);

                item->set_full_name(item_name);

                item->set_display_name(item_node["name"].as<std::string>());

                item->set_description(item_node["description"].as<std::string>());

                item->set_is_obtainable(item_node["obtainable"] ? item_node["obtainable"].as<bool>()
                                                                : true);

                if (item_node["usage"]) {

                    item->set_usage(parse_usage(item_node["usage"].as<std::string>()));
                }

                if (item_node["effects"]) {

                    for (const auto& effect_node : item_node["effects"]) {

                        parse_item_effect(effect_node, *item);
                    }
                }
            }
        }

        if (config["npcs"]) {

            const YAML::Node npcs = config["npcs"];

            if (!npcs.IsMap()) {
                throw std::runtime_error("'Npcs' needs to be a YAML map");
            }

            for (const auto& entry : npcs) {

                const std::string npc_type = entry.first.as<std::string>();

                const YAML::Node npc_node = entry.second;

                const uint64_t id = next_npc_id++;

                npc_ids[npc_type] = id;

                game::Npc* npc = world.add_npcs();

                npc->set_id(id);
                npc->set_type(npc_type);

                npc->set_name(npc_node["name"].as<std::string>());

                npc->set_description(npc_node["description"].as<std::string>());

                npc->set_hp(npc_node["hp"] ? npc_node["hp"].as<uint32_t>() : 0);

                if (npc_node["temperament"]) {

                    npc->set_temperament(
                        parse_temperament(npc_node["temperament"].as<std::string>()));
                }

                if (npc_node["dialogue"]) {

                    for (const auto& dialogue : npc_node["dialogue"]) {

                        npc->add_dialogues(dialogue.as<std::string>());
                    }
                }

                if (npc_node["inventory"]) {

                    for (const auto& inventory_entry : npc_node["inventory"]) {

                        parse_npc_inventory_entry(inventory_entry, *npc, item_ids);
                    }
                }
            }
        }

        if (!config["world"] || !config["world"]["locations"]) {

            throw std::runtime_error("The YAML does not contain world.locations");
        }

        const YAML::Node locations = config["world"]["locations"];

        if (!locations.IsMap()) {
            throw std::runtime_error("'world.locations' needs to be a YAML map");
        }

        // 1. Create all locations

        for (const auto& entry : locations) {

            const std::string location_name = entry.first.as<std::string>();

            const uint64_t id = next_location_id++;

            location_ids[location_name] = id;

            game::Location* location = world.add_locations();

            location->set_id(id);

            location->set_name(location_name);

            location->set_description(entry.second["description"].as<std::string>());

            if (location_name == "start") {
                world.set_start_location_id(id);
            }
        }

        // 2. Add exits, items

        for (const auto& entry : locations) {

            const std::string location_name = entry.first.as<std::string>();

            const YAML::Node location_node = entry.second;

            const uint64_t location_id = location_ids.at(location_name);

            game::Location* location = nullptr;

            for (int i = 0; i < world.locations_size(); ++i) {

                if (world.locations(i).id() == location_id) {

                    location = world.mutable_locations(i);

                    break;
                }
            }

            if (!location) {
                throw std::runtime_error("Impossible to find the location : " + location_name);
            }

            if (location_node["exits"]) {

                const YAML::Node exits = location_node["exits"];

                if (exits.IsMap()) {

                    for (const auto& exit_entry : exits) {

                        const std::string direction = exit_entry.first.as<std::string>();

                        const std::string target = exit_entry.second.as<std::string>();

                        if (!location_ids.contains(target)) {
                            throw std::runtime_error("Unknown target location : " + target);
                        }

                        game::Exit* exit = location->add_exits();

                        exit->set_direction(parse_direction(direction));

                        exit->set_location_id(location_ids.at(target));

                        exit->set_is_locked(false);
                    }
                }

                else if (exits.IsSequence()) {

                    for (const auto& exit_node : exits) {

                        const std::string direction = exit_node["direction"].as<std::string>();

                        const std::string target = exit_node["location_name"].as<std::string>();

                        if (!location_ids.contains(target)) {
                            throw std::runtime_error("Unknown target location : " + target);
                        }

                        game::Exit* exit = location->add_exits();

                        exit->set_direction(parse_direction(direction));

                        exit->set_location_id(location_ids.at(target));

                        const bool locked =
                            exit_node["is_locked"] ? exit_node["is_locked"].as<bool>() : false;

                        exit->set_is_locked(locked);

                        if (exit_node["unlock_item"]) {

                            const std::string item_name =
                                exit_node["unlock_item"].as<std::string>();

                            if (!item_ids.contains(item_name)) {
                                throw std::runtime_error("Unknown unlocking item" + item_name);
                            }

                            exit->set_unlock_item_id(item_ids.at(item_name));
                        }
                    }
                }

                else {
                    throw std::runtime_error("'exits' needs to be a map or a list");
                }
            }

            if (location_node["items"]) {

                for (const auto& item_node : location_node["items"]) {

                    const std::string item_name = item_node.as<std::string>();

                    if (!item_ids.contains(item_name)) {
                        throw std::runtime_error("Unknown item in the location : '" +
                                                 location_name + "' : " + item_name);
                    }

                    location->add_item_ids(item_ids.at(item_name));
                }
            }

            if (location_node["spawns"]) {

                for (const auto& spawn_node : location_node["spawns"]) {

                    const std::string npc_type = spawn_node["npc_type"].as<std::string>();

                    const uint64_t amount =
                        spawn_node["count"] ? spawn_node["count"].as<uint64_t>() : 1;

                    if (!npc_ids.contains(npc_type)) {
                        throw std::runtime_error("Unkown Npc type in the location ; '" +
                                                 location_name + "' : " + npc_type);
                    }

                    game::Spawn* spawn = location->add_spawns();

                    spawn->set_npc_type(npc_type);
                    spawn->set_amount(amount);
                }
            }
        }

        std::cout << "World initialized successfully." << std::endl;

        return true;
    }

    catch (const YAML::BadFile& e) {

        std::cerr << "Impossible to open the YAML file" << e.what() << std::endl;

        return false;
    }

    catch (const YAML::Exception& e) {

        std::cerr << "YAML syntax error : " << e.what() << std::endl;

        return false;
    }

    catch (const std::exception& e) {

        std::cerr << "Error when initializing the world : " << e.what() << std::endl;

        return false;
    }
}

void sendMessageToEveryone(game::WorldDelta& message, std::vector<std::unique_ptr<Runner>>& runners)
{
    for (auto& runner : runners) {
        runner->send_change(message);
    }
}

void server_loop(ThreadSafeQueue<Socket>& pending_connections, game::World& world)
{
    uint64_t next_player_id = {0};
    std::vector<std::unique_ptr<Runner>> runners;

    while (true) {

        Socket socket;
        while (pending_connections.try_pop(socket)) {

            game::Player newPlayer{};
            newPlayer.set_id(next_player_id++);
            game::RcvStatus status{};
            auto command_ = socket.receive_message<game::CommandDelta>(status);

            if (status != game::RcvStatus::OK)
                continue;
            if (!command_.has_value())
                continue;
            if (command_->delta_type_case() != game::CommandDelta::kNameSetCommand)
                continue;

            auto* name_set = (command_.value()).mutable_name_set_command();
            newPlayer.set_name(name_set->name());
            newPlayer.set_hp(100);
            newPlayer.set_money_amount(10);
            newPlayer.set_state(game::PlayerState::CHILLING);

            auto runner = std::make_unique<Runner>(newPlayer.id(), std::move(socket));
            runner->start();

            (*world.mutable_players_locations())[runner->get_player_id()] =
                world.start_location_id();
            *world.add_players() = newPlayer;

            runner->send_world_init(world, newPlayer);
            runners.push_back(std::move(runner));
            std::cout << "New player connected : " << runner->get_player_id() << std::endl;
        }

        for (auto& runner : runners) {

            game::CommandDelta command;

            while (runner->poll_incoming(command)) {

                switch (command.delta_type_case()) {
                // HANDLE INTERACTION COMMAND
                case game::CommandDelta::kInteractionCommand: {
                    // const auto& cmd = command.interaction_command();
                    break;
                }
                // HANDLE ATTACK COMMAND
                case game::CommandDelta::kAttackCommand: {
                    // const auto& cmd = command.attack_command();
                    break;
                }
                // HANDLE MESSAGE COMMAND
                case game::CommandDelta::kMessageCommand: {
                    const auto& cmd = command.message_command();
                    if (runner->is_running()) {
                        game::WorldDelta delta;
                        auto* message_delta = delta.mutable_message_received();
                        message_delta->set_player_id(runner->get_player_id());
                        message_delta->set_message(cmd.message());
                        sendMessageToEveryone(delta, runners);
                    }
                    else {
                        game::WorldDelta delta;
                        auto* unauthorized_command = delta.mutable_unauthorized_command();
                        unauthorized_command->set_response("Cannot send the message!");
                        runner->send_change(delta);
                    }
                    break;
                }
                case game::CommandDelta::DELTA_TYPE_NOT_SET:
                default:
                    break;
                }
            }
        }
    }
}

Socket accept_connection(int socket_fd, struct sockaddr_in& addr)
{
    socklen_t socklen = sizeof(addr);
    Socket socket(accept(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), &socklen));
    return socket;
}

int main()
{

    ThreadSafeQueue<Socket> pending_connections;
    game::World world{};

    // SERVER SOCKET PREPARATION
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1332);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    Socket server_socket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (!server_socket.is_valid())
        return (perror("Socket creation error"), EXIT_FAILURE);
    if (bind(server_socket.get(), reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1)
        return (perror("Bind error"), EXIT_FAILURE);
    if (listen(server_socket.get(), 10) == -1)
        return (perror("Listen error"), EXIT_FAILURE);

    // WORLD INIT
    init_world("world.yaml", world);

    // SERVER LOOP (Takes commands from the queue, do the job and pushes responses
    // to queue)
    std::jthread server_thread([&]() { server_loop(pending_connections, world); });

    // ACCEPT LOOP (Wait for connections and pushes the client's sockets for the
    // server_loop)
    std::jthread accept_thread([&]() {
        while (true) {
            Socket client = accept_connection(server_socket.get(), addr);
            if (client.is_valid()) {
                pending_connections.push(std::move(client));
            }
        }
    });

    return 0;
}
