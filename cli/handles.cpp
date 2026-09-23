#include "cli.hpp"
#include "template.pb.h"

void handleWorldInit(game::WorldDelta& worldInit, game::ClientWorld& localWorld)
{
    auto* world_initiation = worldInit.mutable_world_initiation();

    localWorld.set_start_location_id(world_initiation->start_location_id());
    for (const auto& item : world_initiation->items()) {
        *localWorld.add_items() = item;
    }
    for (const auto& npc : world_initiation->npcs()) {
        *localWorld.add_npcs() = npc;
    }
    for (const auto& location : world_initiation->locations()) {
        *localWorld.add_locations() = location;
    }
    for (const auto& [player_id, player_name] : world_initiation->players()) {
        (*localWorld.mutable_players())[player_id] = player_name;
    }
    for (const auto& [player_id, location_id] : world_initiation->players_locations()) {
        (*localWorld.mutable_players_locations())[player_id] = location_id;
    }
    for (const auto& [player_id, player_hp] : world_initiation->players_hp()) {
        (*localWorld.mutable_players_hp())[player_id] = player_hp;
    }
    for (const auto& [player_id, player_state] : world_initiation->players_states()) {
        (*localWorld.mutable_players_states())[player_id] = player_state;
    }
    *localWorld.mutable_myself() = world_initiation->you();
}

void handleUnauthorizedCommand(game::WorldDelta& change)
{
    auto* delta = change.mutable_unauthorized_command();
    std::cout << "Unauthorized command: " << delta->response() << std::endl;
}

void handlePlayerConnected(game::WorldDelta& change, game::ClientWorld& localWorld)
{
    auto* player_connected = change.mutable_player_connected();
    (*localWorld.mutable_players())[player_connected->player_id()] =
        player_connected->player_name();
}

void handlePlayerDisconnected(game::WorldDelta& change, game::ClientWorld& localWorld)
{
    auto* player_disconnected = change.mutable_player_disconnected();
    (*localWorld.mutable_players()).erase(player_disconnected->player_id());
}

void handleMessageReceived(game::WorldDelta& change, game::ClientWorld& localWorld)
{
    auto* message_received = change.mutable_message_received();
    std::cout << (*localWorld.mutable_players())[message_received->player_id()] << " : "
              << message_received->message() << std::endl;
}

void handlePlayerInventoryChanged(game::WorldDelta& change, game::ClientWorld& localWorld)
{
    auto* inventoryChange = change.mutable_player_inventory_changed();
    if (inventoryChange->player_id() != localWorld.myself().id())
        return;
    auto* myself = localWorld.mutable_myself();
    auto* myInventory = myself->mutable_inventory();
    switch (inventoryChange->interaction()) {
    case game::InteractionType::USE:
    case game::InteractionType::DROP: {
        auto it = std::find(myInventory->begin(), myInventory->end(), inventoryChange->item_id());
        if (it != myInventory->end())
            myInventory->erase(it);
        break;
    }
    case game::InteractionType::PICK_UP:
        myInventory->Add(inventoryChange->item_id());
    default:
        break;
    }
}

void handleNpcInventoryChanged(game::WorldDelta& change, game::ClientWorld& localWorld)
{
    auto* inventoryChange = change.mutable_npc_inventory_changed();
    auto npcFound = std::find_if(
        localWorld.mutable_npcs()->begin(), localWorld.mutable_npcs()->end(),
        [id = inventoryChange->npc_id()](const game::Npc& npc) { return npc.id() == id; });
    if (npcFound == localWorld.npcs().end())
        return;
    auto* inventory = npcFound->mutable_inventory();
    auto itemFound = inventory->find(inventoryChange->item_id());
    if (itemFound == inventory->end())
        return;
    switch (inventoryChange->interaction()) {
    case game::InteractionType::USE:
    case game::InteractionType::DROP:
    case game::InteractionType::GIVE:
        inventory->erase(itemFound);
        break;
    default:
        break;
    }
}

void handlePlayerHpChanged(game::WorldDelta& change, game::ClientWorld& localWorld)
{
    auto* hpChanged = change.mutable_player_hp_changed();

    if (hpChanged->player_id() == localWorld.myself().id())
        localWorld.mutable_myself()->set_hp(hpChanged->new_hp());
    else {
        auto* players_hp = localWorld.mutable_players_hp();
        auto playerFound = players_hp->find(hpChanged->player_id());
        if (playerFound == players_hp->end())
            return;
        playerFound->second = hpChanged->new_hp();
    }
}

void handleNpcHpChanged(game::WorldDelta& change, game::ClientWorld& localWorld)
{
    auto hpChanged = change.mutable_npc_hp_changed();

    auto npcFound =
        std::find_if(localWorld.mutable_npcs()->begin(), localWorld.mutable_npcs()->end(),
                     [id = hpChanged->npc_id()](game::Npc& npc) { return npc.id() == id; });
    if (npcFound == localWorld.mutable_npcs()->end())
        return;
    else {
        npcFound->set_hp(hpChanged->new_hp());
    }
}

void handlePlayerMoved(game::WorldDelta& change, game::ClientWorld& localWorld)
{
    auto* move = change.mutable_player_moved();
    auto playerFound = localWorld.mutable_players_locations()->find(move->player_id());
    if (playerFound == localWorld.players_locations().end())
        return;
    else
        playerFound->second = move->new_location_id();
}

void handleDoorUnlock(game::WorldDelta& change, game::ClientWorld& localWorld)
{
    auto* unlock = change.mutable_door_unlock();

    auto locationFound = std::find_if(
        localWorld.mutable_locations()->begin(), localWorld.mutable_locations()->end(),
        [id = unlock->location_id()](game::Location& location) { return location.id() == id; });
    if (locationFound == localWorld.mutable_locations()->end())
        return;

    auto doorFound =
        std::find_if(locationFound->mutable_exits()->begin(), locationFound->mutable_exits()->end(),
                     [direction = unlock->exit_direction()](game::Exit& exit) {
                         return exit.direction() == direction;
                     });
    if (doorFound == locationFound->mutable_exits()->end())
        return;

    doorFound->set_is_locked(false);
}

void handlePlayerStateChanged(game::WorldDelta& change, game::ClientWorld& localWorld)
{
    auto* stateChanged = change.mutable_player_state_changed();

    if (localWorld.mutable_myself()->id() == stateChanged->player_id())
        localWorld.mutable_myself()->set_state(stateChanged->new_state());
    else {
        auto playerFound = localWorld.mutable_players()->find(stateChanged->player_id());
        if (playerFound == localWorld.mutable_players()->end())
            return;
        playerFound->second = stateChanged->new_state();
    }
}
