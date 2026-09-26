#pragma once

#include "common/Logger.hpp"
#include "common/Protocol.hpp"
#include "common/Socket.hpp"
#include "server/World.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace tap {

struct ItemInstance {
    std::string id;
    std::string type;
};

struct NpcInstance {
    std::string id;
    std::string type;
    int hp = 1;
};

struct QuestProgress {
    enum class Status { Active, Completed };

    Status status = Status::Active;
    std::uint32_t progress = 0;
};

struct Player {
    std::string name;
    std::string room;
    int hp = 100;
    std::vector<ItemInstance> inventory;
    std::map<std::string, QuestProgress> quests;
    std::string group;
    std::set<std::string> invitations;
    std::string combat_target;
    bool defending = false;
    std::shared_ptr<MessageSink> sink;
};

struct CommandResult {
    std::string reply;
    bool quit = false;
};

class Game {
public:
    Game(World world, Logger& logger);

    [[nodiscard]] std::string connect(const std::string& name,
                                      std::shared_ptr<MessageSink> sink);
    void disconnect(const std::string& name);
    [[nodiscard]] CommandResult handle(const std::string& player_name, const Command& command);

private:
    struct RoomState {
        std::vector<ItemInstance> items;
        std::vector<NpcInstance> npcs;
    };

    struct Group {
        std::string id;
        std::string leader;
        std::set<std::string> members;
    };

    std::string look(const Player& player) const;
    std::string move(Player& player, const Command& command);
    std::string chat(Player& player, const Command& command);
    std::string who(const Player& player) const;
    std::string group(Player& player, const Command& command);
    std::string take(Player& player, const Command& command);
    std::string drop(Player& player, const Command& command);
    std::string inventory(const Player& player) const;
    std::string talk(Player& player, const Command& command);
    std::string attack(Player& player, const Command& command);
    std::string defend(Player& player);
    std::string flee(Player& player);
    std::string status(const Player& player) const;
    std::string quest(Player& player, const Command& command);
    std::string quests(const Player& player) const;

    ItemInstance make_item(const std::string& type);
    NpcInstance make_npc(const std::string& type);
    ItemInstance* resolve_inventory_item(Player& player, const std::string& query);
    ItemInstance* resolve_room_item(const Player& player, const std::string& query);
    NpcInstance* resolve_npc(const Player& player, const std::string& query);
    bool matches_item(const ItemInstance& item, const std::string& query) const;
    bool matches_npc(const NpcInstance& npc, const std::string& query) const;
    std::uint32_t fetch_progress(const Player& player, const QuestDefinition& quest) const;
    int attack_damage(const Player& player) const;
    void advance_defeat_quests(Player& player, const std::string& npc_type);
    void respawn_item(const std::string& item_type);
    void remove_from_group(Player& player);

    void send(Player& player, const std::string& message);
    void broadcast_global(const std::string& message, const std::string& excluded = {});
    void broadcast_room(const std::string& room, const std::string& message,
                        const std::string& excluded = {});
    void broadcast_group(const std::string& group_id, const std::string& message,
                         const std::string& excluded = {});
    void broadcast_stats(const std::string& excluded = {});

    World world_;
    Logger& logger_;
    std::map<std::string, RoomState> rooms_;
    std::map<std::string, Player> players_;
    std::map<std::string, Group> groups_;
    std::uint64_t next_item_id_ = 1;
    std::uint64_t next_npc_id_ = 1;
    std::uint64_t next_group_id_ = 1;
    mutable std::mutex mutex_;
};

} // namespace tap
