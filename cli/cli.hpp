#include <arpa/inet.h>
#include <sys/socket.h>

#include "../common/template.pb.h"

void handleWorldInit(game::WorldDelta& worldInit, game::ClientWorld& localWorld);

void handleUnauthorizedCommand(game::WorldDelta& change);

void handlePlayerConnected(game::WorldDelta& change, game::ClientWorld& localWorld);

void handlePlayerDisconnected(game::WorldDelta& change, game::ClientWorld& localWorld);

void handleMessageReceived(game::WorldDelta& change, game::ClientWorld& localWorld);

void handlePlayerInventoryChanged(game::WorldDelta& change, game::ClientWorld& localWorld);

void handleNpcInventoryChanged(game::WorldDelta& change, game::ClientWorld& localWorld);

void handlePlayerHpChanged(game::WorldDelta& change, game::ClientWorld& localWorld);

void handleNpcHpChanged(game::WorldDelta& change, game::ClientWorld& localWorld);

void handlePlayerMoved(game::WorldDelta& change, game::ClientWorld& localWorld);

void handleDoorUnlock(game::WorldDelta& change, game::ClientWorld& localWorld);

void handlePlayerStateChanged(game::WorldDelta& change, game::ClientWorld& localWorld);
