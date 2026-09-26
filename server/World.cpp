#include "server/World.hpp"

#include "common/Protocol.hpp"

#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace tap {
namespace {

std::string required_string(const YAML::Node& node, const char* key, std::string_view context)
{
    if (!node[key] || !node[key].IsScalar())
        throw std::runtime_error(std::string(context) + " requires '" + key + "'");
    return node[key].as<std::string>();
}

std::vector<std::string> string_list(const YAML::Node& node)
{
    std::vector<std::string> values;
    if (!node)
        return values;
    if (!node.IsSequence())
        throw std::runtime_error("Expected a YAML sequence");
    for (const auto& value : node)
        values.push_back(value.as<std::string>());
    return values;
}

template <typename Map>
void require_reference(const Map& values, const std::string& key, const std::string& context)
{
    if (!values.contains(key))
        throw std::runtime_error(context + " references unknown id '" + key + "'");
}

} // namespace

World World::load(const std::string& path)
{
    const YAML::Node root = YAML::LoadFile(path);
    World world;
    world.start_room = required_string(root, "start_room", "world");
    world.respawn_room = required_string(root, "respawn_room", "world");

    if (!root["items"] || !root["items"].IsMap())
        throw std::runtime_error("'items' must be a YAML map");
    for (const auto& entry : root["items"]) {
        ItemDefinition item;
        item.id = entry.first.as<std::string>();
        const YAML::Node node = entry.second;
        item.name = required_string(node, "name", item.id);
        item.description = required_string(node, "description", item.id);
        item.obtainable = node["obtainable"] ? node["obtainable"].as<bool>() : true;
        item.heal = node["heal"] ? node["heal"].as<int>() : 0;
        item.attack_bonus = node["attack_bonus"] ? node["attack_bonus"].as<int>() : 0;
        world.items.emplace(item.id, std::move(item));
    }

    if (!root["npcs"] || !root["npcs"].IsMap())
        throw std::runtime_error("'npcs' must be a YAML map");
    for (const auto& entry : root["npcs"]) {
        NpcDefinition npc;
        npc.id = entry.first.as<std::string>();
        const YAML::Node node = entry.second;
        npc.name = required_string(node, "name", npc.id);
        npc.description = required_string(node, "description", npc.id);
        npc.dialogue = string_list(node["dialogue"]);
        npc.hostile = node["hostile"] ? node["hostile"].as<bool>() : false;
        npc.hp = node["hp"] ? node["hp"].as<int>() : 1;
        npc.attack = node["attack"] ? node["attack"].as<int>() : 0;
        npc.quest = node["quest"] ? node["quest"].as<std::string>() : "";
        world.npcs.emplace(npc.id, std::move(npc));
    }

    if (!root["quests"] || !root["quests"].IsMap())
        throw std::runtime_error("'quests' must be a YAML map");
    for (const auto& entry : root["quests"]) {
        QuestDefinition quest;
        quest.id = entry.first.as<std::string>();
        const YAML::Node node = entry.second;
        quest.giver = required_string(node, "giver", quest.id);
        quest.description = required_string(node, "description", quest.id);
        const std::string kind = required_string(node, "kind", quest.id);
        if (kind == "fetch")
            quest.kind = QuestDefinition::Kind::Fetch;
        else if (kind == "defeat")
            quest.kind = QuestDefinition::Kind::Defeat;
        else
            throw std::runtime_error(quest.id + " has invalid quest kind");
        quest.target = required_string(node, "target", quest.id);
        quest.amount = node["amount"] ? node["amount"].as<std::uint32_t>() : 1;
        quest.reward = required_string(node, "reward", quest.id);
        world.quests.emplace(quest.id, std::move(quest));
    }

    if (!root["rooms"] || !root["rooms"].IsMap())
        throw std::runtime_error("'rooms' must be a YAML map");
    for (const auto& entry : root["rooms"]) {
        RoomDefinition room;
        room.id = entry.first.as<std::string>();
        const YAML::Node node = entry.second;
        room.name = required_string(node, "name", room.id);
        room.description = required_string(node, "description", room.id);
        if (node["exits"]) {
            if (!node["exits"].IsMap())
                throw std::runtime_error(room.id + ".exits must be a map");
            for (const auto& exit : node["exits"])
                room.exits.emplace(lower_copy(exit.first.as<std::string>()),
                                   exit.second.as<std::string>());
        }
        room.items = string_list(node["items"]);
        room.npcs = string_list(node["npcs"]);
        world.rooms.emplace(room.id, std::move(room));
    }

    world.validate();
    return world;
}

void World::validate() const
{
    require_reference(rooms, start_room, "start_room");
    require_reference(rooms, respawn_room, "respawn_room");
    if (rooms.size() < 8)
        throw std::runtime_error("The world requires at least 8 rooms");
    if (items.size() < 4)
        throw std::runtime_error("The world requires at least 4 item types");
    if (quests.size() < 2)
        throw std::runtime_error("The world requires at least 2 quests");

    std::size_t obtainable_spawns = 0;
    for (const auto& [room_id, room] : rooms) {
        for (const auto& [direction, destination] : room.exits) {
            if (direction.empty())
                throw std::runtime_error(room_id + " has an empty direction");
            require_reference(rooms, destination, room_id);
        }
        for (const std::string& item : room.items) {
            require_reference(items, item, room_id);
            if (items.at(item).obtainable)
                ++obtainable_spawns;
        }
        for (const std::string& npc : room.npcs)
            require_reference(npcs, npc, room_id);
    }
    if (obtainable_spawns < 2)
        throw std::runtime_error("The world requires at least 2 obtainable item spawns");

    for (const auto& [npc_id, npc] : npcs) {
        if (npc.hp <= 0 || npc.attack < 0)
            throw std::runtime_error(npc_id + " has invalid combat stats");
        if (!npc.quest.empty())
            require_reference(quests, npc.quest, npc_id);
    }
    for (const auto& [quest_id, quest] : quests) {
        require_reference(npcs, quest.giver, quest_id);
        require_reference(items, quest.reward, quest_id);
        if (quest.amount == 0)
            throw std::runtime_error(quest_id + " has a zero amount");
        if (quest.kind == QuestDefinition::Kind::Fetch)
            require_reference(items, quest.target, quest_id);
        else
            require_reference(npcs, quest.target, quest_id);
        if (npcs.at(quest.giver).quest != quest_id)
            throw std::runtime_error(quest_id + " is not assigned to its giver");
    }
}

} // namespace tap
