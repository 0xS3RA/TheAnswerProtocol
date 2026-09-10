#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>

#include "../common/Socket.hpp"

void handleWorldInit(game::WorldDelta& worldInit, game::ClientWorld& localWorld);

void handleUnauthorizedCommand(game::WorldDelta& change);

void handlePlayerConnected(game::WorldDelta& change, game::ClientWorld& localWorld);

void handlePlayerDisconnected(game::WorldDelta& change, game::ClientWorld& localWorld);

void handleMessageReceived(game::WorldDelta& change, game::ClientWorld& localWorld);

void handlePlayerInventoryChanged(game::WorldDelta& change, game::ClientWorld& localWorld);
