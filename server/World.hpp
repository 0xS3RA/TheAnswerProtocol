#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace tap {

struct ItemDefinition {
    std::string id;
    std::string name;
    std::string description;
    bool obtainable = true;
    int heal = 0;
    int attack_bonus = 0;
};

struct NpcDefinition {
    std::string id;
    std::string name;
    std::string description;
    std::vector<std::string> dialogue;
    bool hostile = false;
    int hp = 1;
    int attack = 0;
    std::string quest;
};

struct QuestDefinition {
    enum class Kind { Fetch, Defeat };

    std::string id;
    std::string giver;
    std::string description;
    Kind kind = Kind::Fetch;
    std::string target;
    std::uint32_t amount = 1;
    std::string reward;
};

struct RoomDefinition {
    std::string id;
    std::string name;
    std::string description;
    std::map<std::string, std::string> exits;
    std::vector<std::string> items;
    std::vector<std::string> npcs;
};

struct World {
    std::string start_room;
    std::string respawn_room;
    std::map<std::string, ItemDefinition> items;
    std::map<std::string, NpcDefinition> npcs;
    std::map<std::string, QuestDefinition> quests;
    std::map<std::string, RoomDefinition> rooms;

    static World load(const std::string& path);
    void validate() const;
};

} // namespace tap
