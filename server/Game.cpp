#include "server/Game.hpp"

#include <algorithm>
#include <sstream>

namespace tap {
namespace {

constexpr int bad_request = 400;
constexpr int not_connected = 202;
constexpr int no_exit = 301;
constexpr int not_in_group = 401;
constexpr int already_in_group = 402;
constexpr int not_found = 404;
constexpr int npc_not_hostile = 405;
constexpr int no_quest = 406;
constexpr int not_in_combat = 407;
constexpr int target_missing = 408;

std::string string_array(const std::vector<std::string>& values)
{
    std::string json = "[";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0)
            json += ',';
        json += json_string(values[index]);
    }
    return json + ']';
}

std::string suffix(std::string_view id)
{
    const std::size_t position = id.rfind('.');
    return std::string(position == std::string_view::npos ? id : id.substr(position + 1));
}

} // namespace

Game::Game(World world, Logger& logger) : world_(std::move(world)), logger_(logger)
{
    for (const auto& [room_id, definition] : world_.rooms) {
        RoomState state;
        for (const std::string& item : definition.items)
            state.items.push_back(make_item(item));
        for (const std::string& npc : definition.npcs)
            state.npcs.push_back(make_npc(npc));
        rooms_.emplace(room_id, std::move(state));
    }
}

std::string Game::connect(const std::string& name, std::shared_ptr<MessageSink> sink)
{
    std::lock_guard lock(mutex_);
    if (name.empty() || name.size() > 32 || !is_safe_text(name) ||
        std::any_of(name.begin(), name.end(), [](unsigned char value) { return std::isspace(value); }))
        return error(bad_request, "BAD_REQUEST");
    if (players_.contains(name))
        return error(201, "NAME_IN_USE");

    Player player;
    player.name = name;
    player.room = world_.start_room;
    player.sink = std::move(sink);
    players_.emplace(name, std::move(player));
    logger_.info("player_connected", {{"player", name}, {"room", world_.start_room}});
    broadcast_room(world_.start_room, event("ROOM PRESENCE ENTER", name), name);
    broadcast_stats(name);
    return ok("connected");
}

void Game::disconnect(const std::string& name)
{
    std::lock_guard lock(mutex_);
    auto iterator = players_.find(name);
    if (iterator == players_.end())
        return;

    const std::string room = iterator->second.room;
    const std::string group_id = iterator->second.group;
    std::vector<std::string> dropped_items;
    for (ItemInstance& item : iterator->second.inventory) {
        dropped_items.push_back(item.id);
        rooms_.at(room).items.push_back(std::move(item));
    }
    remove_from_group(iterator->second);
    players_.erase(iterator);
    logger_.info("player_disconnected", {{"player", name}, {"room", room}});
    if (!group_id.empty())
        broadcast_group(group_id, event("GROUP LEAVE", name));
    for (const std::string& item : dropped_items) {
        logger_.info("item_drop",
                     {{"player", name}, {"item", item}, {"room", room}, {"reason", "disconnect"}});
        broadcast_room(room, event("ROOM ITEM", name + " dropped " + item));
    }
    broadcast_room(room, event("ROOM PRESENCE LEAVE", name));
    broadcast_stats();
}

CommandResult Game::handle(const std::string& player_name, const Command& command)
{
    std::lock_guard lock(mutex_);
    auto iterator = players_.find(player_name);
    if (iterator == players_.end())
        return {error(not_connected, "NOT_CONNECTED"), true};

    Player& player = iterator->second;
    logger_.info("command", {{"player", player.name},
                             {"command", command.name},
                             {"arguments", command.rest(0)}});

    CommandResult result;
    if (command.name == "LOOK")
        result.reply = command.arguments.empty() ? look(player) : error(bad_request, "BAD_REQUEST");
    else if (command.name == "MOVE")
        result.reply = move(player, command);
    else if (command.name == "CHAT")
        result.reply = chat(player, command);
    else if (command.name == "WHO")
        result.reply = command.arguments.empty() ? who(player) : error(bad_request, "BAD_REQUEST");
    else if (command.name == "GROUP")
        result.reply = group(player, command);
    else if (command.name == "TAKE")
        result.reply = take(player, command);
    else if (command.name == "DROP")
        result.reply = drop(player, command);
    else if (command.name == "INVENTORY")
        result.reply =
            command.arguments.empty() ? inventory(player) : error(bad_request, "BAD_REQUEST");
    else if (command.name == "TALK")
        result.reply = talk(player, command);
    else if (command.name == "ATTACK")
        result.reply = attack(player, command);
    else if (command.name == "DEFEND")
        result.reply =
            command.arguments.empty() ? defend(player) : error(bad_request, "BAD_REQUEST");
    else if (command.name == "FLEE")
        result.reply =
            command.arguments.empty() ? flee(player) : error(bad_request, "BAD_REQUEST");
    else if (command.name == "STATUS")
        result.reply =
            command.arguments.empty() ? status(player) : error(bad_request, "BAD_REQUEST");
    else if (command.name == "QUEST")
        result.reply = quest(player, command);
    else if (command.name == "QUESTS")
        result.reply =
            command.arguments.empty() ? quests(player) : error(bad_request, "BAD_REQUEST");
    else if (command.name == "QUIT")
        result = {command.arguments.empty() ? ok("bye") : error(bad_request, "BAD_REQUEST"),
                  command.arguments.empty()};
    else if (command.name == "CONNECT")
        result.reply = error(203, "ALREADY_CONNECTED");
    else
        result.reply = error(bad_request, "BAD_REQUEST");

    if (result.reply.starts_with("ERR"))
        logger_.warn("response_error", {{"player", player.name}, {"response", result.reply}});
    else
        logger_.info("response", {{"player", player.name}, {"response", result.reply}});
    return result;
}

std::string Game::look(const Player& player) const
{
    const RoomDefinition& room = world_.rooms.at(player.room);
    std::string exits = "{";
    bool first = true;
    for (const auto& [direction, destination] : room.exits) {
        if (!first)
            exits += ',';
        first = false;
        exits += json_string(direction) + ':' + json_string(destination);
    }
    exits += '}';

    std::vector<std::string> player_names;
    for (const auto& [name, other] : players_) {
        if (other.room == player.room)
            player_names.push_back(name);
    }
    std::vector<std::string> item_ids;
    for (const ItemInstance& item : rooms_.at(player.room).items)
        item_ids.push_back(item.id);
    std::vector<std::string> npc_ids;
    for (const NpcInstance& npc : rooms_.at(player.room).npcs)
        npc_ids.push_back(npc.id);

    const std::string json =
        "{\"room\":{\"id\":" + json_string(room.id) + ",\"name\":" + json_string(room.name) +
        ",\"description\":" + json_string(room.description) + ",\"exits\":" + exits +
        "},\"players\":" + string_array(player_names) + ",\"items\":" +
        string_array(item_ids) + ",\"npcs\":" + string_array(npc_ids) + '}';
    return ok(json);
}

std::string Game::move(Player& player, const Command& command)
{
    if (command.arguments.size() != 1)
        return error(command.arguments.empty() ? target_missing : bad_request,
                     command.arguments.empty() ? "TARGET_MISSING" : "BAD_REQUEST");
    const std::string direction = lower_copy(command.arguments.front());
    const RoomDefinition& current = world_.rooms.at(player.room);
    const auto exit = current.exits.find(direction);
    if (exit == current.exits.end())
        return error(no_exit, "NO_EXIT");

    const std::string previous = player.room;
    player.room = exit->second;
    player.combat_target.clear();
    player.defending = false;
    broadcast_room(previous, event("ROOM PRESENCE LEAVE", player.name), player.name);
    broadcast_room(player.room, event("ROOM PRESENCE ENTER", player.name), player.name);
    logger_.info("move", {{"player", player.name}, {"from", previous}, {"to", player.room}});
    return ok("room=" + player.room);
}

std::string Game::chat(Player& player, const Command& command)
{
    if (command.arguments.size() < 2 || command.rest(1).empty())
        return error(target_missing, "TARGET_MISSING");
    const std::string scope = lower_copy(command.arguments[0]);
    const std::string message = command.rest(1);
    if (!is_safe_text(message))
        return error(bad_request, "BAD_REQUEST");

    if (scope == "global")
        broadcast_global(event("GLOBAL CHAT", player.name + " " + message));
    else if (scope == "room")
        broadcast_room(player.room, event("ROOM CHAT", player.name + " " + message));
    else if (scope == "group") {
        if (player.group.empty())
            return error(not_in_group, "NOT_IN_GROUP");
        broadcast_group(player.group, event("GROUP CHAT", player.name + " " + message));
    }
    else
        return error(bad_request, "BAD_REQUEST");
    logger_.info("chat", {{"player", player.name}, {"scope", scope}});
    return ok();
}

std::string Game::who(const Player&) const
{
    return ok("players=" + std::to_string(players_.size()));
}

std::string Game::group(Player& player, const Command& command)
{
    if (command.arguments.empty())
        return error(bad_request, "BAD_REQUEST");
    const std::string operation = lower_copy(command.arguments[0]);
    if (operation == "create") {
        if (command.arguments.size() != 1)
            return error(bad_request, "BAD_REQUEST");
        if (!player.group.empty())
            return error(already_in_group, "ALREADY_IN_GROUP");
        Group created;
        created.id = "group." + std::to_string(next_group_id_++);
        created.leader = player.name;
        created.members.insert(player.name);
        player.group = created.id;
        groups_.emplace(created.id, created);
        logger_.info("group_created", {{"player", player.name}, {"group", created.id}});
        return ok("group=" + created.id);
    }
    if (operation == "invite") {
        if (command.arguments.size() != 2)
            return error(target_missing, "TARGET_MISSING");
        if (player.group.empty())
            return error(not_in_group, "NOT_IN_GROUP");
        Group& current = groups_.at(player.group);
        if (current.leader != player.name)
            return error(not_in_group, "NOT_GROUP_LEADER");
        auto target = players_.find(command.arguments[1]);
        if (target == players_.end())
            return error(not_found, "PLAYER_NOT_FOUND");
        target->second.invitations.insert(player.group);
        send(target->second, event("GROUP INVITE", player.name));
        return ok();
    }
    if (operation == "join") {
        if (command.arguments.size() != 2)
            return error(target_missing, "TARGET_MISSING");
        if (!player.group.empty())
            return error(already_in_group, "ALREADY_IN_GROUP");
        auto leader = players_.find(command.arguments[1]);
        if (leader == players_.end() || leader->second.group.empty() ||
            !player.invitations.contains(leader->second.group))
            return error(not_found, "GROUP_INVITE_NOT_FOUND");
        Group& joined = groups_.at(leader->second.group);
        player.group = joined.id;
        player.invitations.erase(joined.id);
        joined.members.insert(player.name);
        broadcast_group(joined.id, event("GROUP JOIN", player.name), player.name);
        return ok("group=" + joined.id);
    }
    if (operation == "leave") {
        if (command.arguments.size() != 1)
            return error(bad_request, "BAD_REQUEST");
        if (player.group.empty())
            return error(not_in_group, "NOT_IN_GROUP");
        const std::string id = player.group;
        remove_from_group(player);
        broadcast_group(id, event("GROUP LEAVE", player.name));
        return ok();
    }
    return error(bad_request, "BAD_REQUEST");
}

std::string Game::take(Player& player, const Command& command)
{
    const std::string query = command.rest(0);
    if (query.empty())
        return error(target_missing, "TARGET_MISSING");
    ItemInstance* item = resolve_room_item(player, query);
    if (item == nullptr || !world_.items.at(item->type).obtainable)
        return error(not_found, "ITEM_NOT_FOUND");
    const std::string id = item->id;
    auto& floor = rooms_.at(player.room).items;
    const auto iterator = std::find_if(floor.begin(), floor.end(),
                                       [&id](const ItemInstance& value) { return value.id == id; });
    player.inventory.push_back(std::move(*iterator));
    floor.erase(iterator);
    broadcast_room(player.room, event("ROOM ITEM", player.name + " took " + id), player.name);
    logger_.info("item_take", {{"player", player.name}, {"item", id}, {"room", player.room}});
    return ok("taken=" + id);
}

std::string Game::drop(Player& player, const Command& command)
{
    const std::string query = command.rest(0);
    if (query.empty())
        return error(target_missing, "TARGET_MISSING");
    ItemInstance* item = resolve_inventory_item(player, query);
    if (item == nullptr)
        return error(not_found, "ITEM_NOT_IN_INVENTORY");
    const std::string id = item->id;
    const auto iterator =
        std::find_if(player.inventory.begin(), player.inventory.end(),
                     [&id](const ItemInstance& value) { return value.id == id; });
    rooms_.at(player.room).items.push_back(std::move(*iterator));
    player.inventory.erase(iterator);
    broadcast_room(player.room, event("ROOM ITEM", player.name + " dropped " + id), player.name);
    logger_.info("item_drop", {{"player", player.name}, {"item", id}, {"room", player.room}});
    return ok("dropped=" + id);
}

std::string Game::inventory(const Player& player) const
{
    std::vector<std::string> ids;
    for (const ItemInstance& item : player.inventory)
        ids.push_back(item.id);
    return ok(string_array(ids));
}

std::string Game::talk(Player& player, const Command& command)
{
    const std::string query = command.rest(0);
    if (query.empty())
        return error(target_missing, "TARGET_MISSING");
    NpcInstance* npc = resolve_npc(player, query);
    if (npc == nullptr)
        return error(not_found, "NPC_NOT_FOUND");
    const NpcDefinition& definition = world_.npcs.at(npc->type);
    const std::string dialogue =
        definition.dialogue.empty() ? "..." : definition.dialogue.front();
    logger_.info("npc_talk", {{"player", player.name}, {"npc", npc->id}});
    return ok(dialogue);
}

std::string Game::attack(Player& player, const Command& command)
{
    const std::string query = command.rest(0);
    if (query.empty())
        return error(target_missing, "TARGET_MISSING");
    NpcInstance* npc = resolve_npc(player, query);
    if (npc == nullptr)
        return error(not_found, "NPC_NOT_FOUND");
    const NpcDefinition& definition = world_.npcs.at(npc->type);
    if (!definition.hostile)
        return error(npc_not_hostile, "NPC_NOT_HOSTILE");

    const std::string combat_room = player.room;
    player.combat_target = npc->id;
    const int damage = attack_damage(player);
    npc->hp = std::max(0, npc->hp - damage);
    int target_hp = npc->hp;
    std::string combat_status = "combat";
    if (npc->hp == 0) {
        const std::string npc_id = npc->id;
        const std::string npc_type = npc->type;
        auto& npcs = rooms_.at(player.room).npcs;
        npcs.erase(std::remove_if(npcs.begin(), npcs.end(),
                                  [&npc_id](const NpcInstance& value) {
                                      return value.id == npc_id;
                                  }),
                   npcs.end());
        for (auto& entry : players_) {
            Player& other = entry.second;
            if (other.combat_target == npc_id) {
                other.combat_target.clear();
                other.defending = false;
            }
        }
        NpcInstance replacement = make_npc(npc_type);
        const std::string replacement_id = replacement.id;
        npcs.push_back(std::move(replacement));
        player.combat_target.clear();
        player.defending = false;
        advance_defeat_quests(player, npc_type);
        combat_status = "victory";
        broadcast_room(combat_room,
                       event("ROOM COMBAT", player.name + " defeated " + npc_id), player.name);
        logger_.info("combat_victory", {{"player", player.name}, {"npc", npc_id}});
        logger_.info(
            "npc_respawn",
            {{"npc", replacement_id}, {"type", npc_type}, {"room", combat_room}});
    }
    else {
        int counter = definition.attack;
        if (player.defending) {
            counter = std::max(1, counter / 2);
            player.defending = false;
        }
        player.hp = std::max(0, player.hp - counter);
        if (player.hp == 0) {
            const std::string old_room = player.room;
            player.room = world_.respawn_room;
            player.hp = 50;
            player.combat_target.clear();
            combat_status = "defeated";
            broadcast_room(old_room, event("ROOM PRESENCE LEAVE", player.name), player.name);
            broadcast_room(player.room, event("ROOM PRESENCE ENTER", player.name), player.name);
            logger_.warn("player_respawn", {{"player", player.name}, {"room", player.room}});
        }
        logger_.info("combat_counter",
                     {{"player", player.name}, {"npc", npc->id}, {"damage", std::to_string(counter)}});
        broadcast_room(combat_room,
                       event("ROOM COMBAT",
                             player.name + " hit " + npc->id + " for " +
                                 std::to_string(damage) + "; counter=" +
                                 std::to_string(counter)),
                       player.name);
    }

    logger_.info("combat_attack",
                 {{"player", player.name}, {"npc", query}, {"damage", std::to_string(damage)}});
    return ok("{\"attacker_hp\":" + std::to_string(player.hp) +
              ",\"target_hp\":" + std::to_string(target_hp) +
              ",\"damage\":" + std::to_string(damage) +
              ",\"status\":" + json_string(combat_status) + '}');
}

std::string Game::defend(Player& player)
{
    if (player.combat_target.empty() || resolve_npc(player, player.combat_target) == nullptr) {
        player.combat_target.clear();
        return error(not_in_combat, "NOT_IN_COMBAT");
    }
    player.defending = true;
    logger_.info("combat_defend", {{"player", player.name}});
    return ok("defending");
}

std::string Game::flee(Player& player)
{
    if (player.combat_target.empty())
        return error(not_in_combat, "NOT_IN_COMBAT");
    player.combat_target.clear();
    player.defending = false;
    logger_.info("combat_flee", {{"player", player.name}});
    return ok("fled");
}

std::string Game::status(const Player& player) const
{
    std::string state = "healthy";
    if (!player.combat_target.empty())
        state = "combat";
    else if (player.hp < 50)
        state = "wounded";
    return ok("{\"hp\":" + std::to_string(player.hp) +
              ",\"max_hp\":100,\"status\":" + json_string(state) + '}');
}

std::string Game::quest(Player& player, const Command& command)
{
    const std::string query = command.rest(0);
    if (query.empty())
        return error(target_missing, "TARGET_MISSING");
    NpcInstance* npc = resolve_npc(player, query);
    if (npc == nullptr)
        return error(not_found, "NPC_NOT_FOUND");
    const NpcDefinition& giver = world_.npcs.at(npc->type);
    if (giver.quest.empty())
        return error(no_quest, "NO_QUEST_AVAILABLE");

    const QuestDefinition& definition = world_.quests.at(giver.quest);
    auto progress = player.quests.find(definition.id);
    if (progress == player.quests.end()) {
        QuestProgress started;
        if (definition.kind == QuestDefinition::Kind::Fetch)
            started.progress = fetch_progress(player, definition);
        player.quests.emplace(definition.id, started);
        logger_.info("quest_accepted", {{"player", player.name}, {"quest", definition.id}});
        return ok("{\"quest_id\":" + json_string(definition.id) +
                  ",\"description\":" + json_string(definition.description) +
                  ",\"reward\":" + json_string(definition.reward) +
                  ",\"status\":\"active\"}");
    }
    if (progress->second.status == QuestProgress::Status::Completed)
        return error(no_quest, "NO_QUEST_AVAILABLE");
    if (definition.kind == QuestDefinition::Kind::Fetch) {
        const std::uint32_t previous = progress->second.progress;
        progress->second.progress = fetch_progress(player, definition);
        if (progress->second.progress != previous)
            logger_.info("quest_progress",
                         {{"player", player.name},
                          {"quest", definition.id},
                          {"progress", std::to_string(progress->second.progress)}});
    }
    if (progress->second.progress < definition.amount) {
        return ok("{\"quest_id\":" + json_string(definition.id) +
                  ",\"status\":\"active\",\"progress\":" +
                  json_string(std::to_string(progress->second.progress) + "/" +
                              std::to_string(definition.amount)) +
                  '}');
    }

    if (definition.kind == QuestDefinition::Kind::Fetch) {
        std::uint32_t removed = 0;
        player.inventory.erase(
            std::remove_if(player.inventory.begin(), player.inventory.end(),
                           [&](const ItemInstance& item) {
                               if (removed < definition.amount && item.type == definition.target) {
                                   ++removed;
                                   return true;
                               }
                               return false;
                           }),
            player.inventory.end());
        for (std::uint32_t count = 0; count < removed; ++count)
            respawn_item(definition.target);
    }
    player.inventory.push_back(make_item(definition.reward));
    progress->second.status = QuestProgress::Status::Completed;
    send(player, event("QUEST COMPLETE", definition.id));
    logger_.info("quest_completed", {{"player", player.name}, {"quest", definition.id}});
    return ok("{\"quest_id\":" + json_string(definition.id) +
              ",\"reward\":" + json_string(definition.reward) +
              ",\"status\":\"completed\"}");
}

std::string Game::quests(const Player& player) const
{
    std::string json = "[";
    bool first = true;
    for (const auto& [id, progress] : player.quests) {
        if (!first)
            json += ',';
        first = false;
        const QuestDefinition& definition = world_.quests.at(id);
        const std::uint32_t value =
            definition.kind == QuestDefinition::Kind::Fetch &&
                    progress.status == QuestProgress::Status::Active
                ? fetch_progress(player, definition)
                : progress.progress;
        json += "{\"quest_id\":" + json_string(id) + ",\"status\":" +
                json_string(progress.status == QuestProgress::Status::Completed ? "completed"
                                                                                : "active") +
                ",\"progress\":" +
                json_string(std::to_string(value) + "/" + std::to_string(definition.amount)) + '}';
    }
    return ok(json + ']');
}

ItemInstance Game::make_item(const std::string& type)
{
    return {type + "." + std::to_string(next_item_id_++), type};
}

NpcInstance Game::make_npc(const std::string& type)
{
    return {type + "." + std::to_string(next_npc_id_++), type, world_.npcs.at(type).hp};
}

ItemInstance* Game::resolve_inventory_item(Player& player, const std::string& query)
{
    const auto iterator =
        std::find_if(player.inventory.begin(), player.inventory.end(),
                     [&](const ItemInstance& item) { return matches_item(item, query); });
    return iterator == player.inventory.end() ? nullptr : &*iterator;
}

ItemInstance* Game::resolve_room_item(const Player& player, const std::string& query)
{
    auto& items = rooms_.at(player.room).items;
    const auto iterator = std::find_if(items.begin(), items.end(),
                                       [&](const ItemInstance& item) {
                                           return matches_item(item, query);
                                       });
    return iterator == items.end() ? nullptr : &*iterator;
}

NpcInstance* Game::resolve_npc(const Player& player, const std::string& query)
{
    auto& npcs = rooms_.at(player.room).npcs;
    const auto iterator = std::find_if(npcs.begin(), npcs.end(),
                                       [&](const NpcInstance& npc) {
                                           return matches_npc(npc, query);
                                       });
    return iterator == npcs.end() ? nullptr : &*iterator;
}

bool Game::matches_item(const ItemInstance& item, const std::string& query) const
{
    const ItemDefinition& definition = world_.items.at(item.type);
    return iequals(item.id, query) || iequals(item.type, query) ||
           iequals(suffix(item.type), query) || iequals(definition.name, query);
}

bool Game::matches_npc(const NpcInstance& npc, const std::string& query) const
{
    const NpcDefinition& definition = world_.npcs.at(npc.type);
    return iequals(npc.id, query) || iequals(npc.type, query) ||
           iequals(suffix(npc.type), query) || iequals(definition.name, query);
}

std::uint32_t Game::fetch_progress(const Player& player, const QuestDefinition& quest) const
{
    return static_cast<std::uint32_t>(
        std::count_if(player.inventory.begin(), player.inventory.end(),
                      [&](const ItemInstance& item) { return item.type == quest.target; }));
}

int Game::attack_damage(const Player& player) const
{
    int damage = 12;
    for (const ItemInstance& item : player.inventory)
        damage += world_.items.at(item.type).attack_bonus;
    return damage;
}

void Game::advance_defeat_quests(Player& player, const std::string& npc_type)
{
    for (auto& [id, progress] : player.quests) {
        const QuestDefinition& quest = world_.quests.at(id);
        if (progress.status == QuestProgress::Status::Active &&
            quest.kind == QuestDefinition::Kind::Defeat && quest.target == npc_type) {
            progress.progress = std::min(quest.amount, progress.progress + 1);
            if (progress.progress == quest.amount)
                send(player, event("QUEST OBJECTIVE", id + " ready"));
            logger_.info("quest_progress",
                         {{"player", player.name},
                          {"quest", id},
                          {"progress", std::to_string(progress.progress)}});
        }
    }
}

void Game::respawn_item(const std::string& item_type)
{
    const auto origin = std::find_if(world_.rooms.begin(), world_.rooms.end(),
                                     [&](const auto& entry) {
                                         return std::find(entry.second.items.begin(),
                                                          entry.second.items.end(),
                                                          item_type) != entry.second.items.end();
                                     });
    if (origin == world_.rooms.end())
        return;
    ItemInstance item = make_item(item_type);
    const std::string id = item.id;
    rooms_.at(origin->first).items.push_back(std::move(item));
    logger_.info("item_respawn", {{"item", id}, {"room", origin->first}});
}

void Game::remove_from_group(Player& player)
{
    if (player.group.empty())
        return;
    const std::string id = player.group;
    auto iterator = groups_.find(id);
    player.group.clear();
    if (iterator == groups_.end())
        return;
    iterator->second.members.erase(player.name);
    if (iterator->second.members.empty()) {
        groups_.erase(iterator);
        for (auto& entry : players_)
            entry.second.invitations.erase(id);
        return;
    }
    if (iterator->second.leader == player.name)
        iterator->second.leader = *iterator->second.members.begin();
}

void Game::send(Player& player, const std::string& message)
{
    if (!player.sink->enqueue(message))
        logger_.warn("send_queue_full", {{"player", player.name}});
}

void Game::broadcast_global(const std::string& message, const std::string& excluded)
{
    for (auto& [name, player] : players_) {
        if (name != excluded)
            send(player, message);
    }
}

void Game::broadcast_room(const std::string& room, const std::string& message,
                          const std::string& excluded)
{
    for (auto& [name, player] : players_) {
        if (name != excluded && player.room == room)
            send(player, message);
    }
}

void Game::broadcast_group(const std::string& group_id, const std::string& message,
                           const std::string& excluded)
{
    for (auto& [name, player] : players_) {
        if (name != excluded && player.group == group_id)
            send(player, message);
    }
}

void Game::broadcast_stats(const std::string& excluded)
{
    broadcast_global(event("STATS", "players=" + std::to_string(players_.size())), excluded);
}

} // namespace tap
