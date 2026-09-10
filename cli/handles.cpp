#include "cli.hpp"
#include "common/template.pb.h"

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
}
