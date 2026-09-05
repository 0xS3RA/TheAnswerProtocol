#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>


enum RcvStatus {
  Disconnected,
  WouldBlock,
  ParseError,
  Ok
};

enum CommandType {
  Interaction,
  Attack,
  Message,
  Invalid
};

enum PlayerState {
  inCombat,
  chilling,
  dead
};

enum InteractionType {
  move,
  pickUp,
  drop,
  open,
  speak,
  attack,
  trade,
  acceptQuest
};

enum CombatAction {
  lightAttack,
  strongAttack,
  concentrate,
  flee,
  halfDefend,
  fullDefend,
  counter,
  none
};

enum Direction {
    north,
    east,
    west,
    south
};

enum EffectType {
  heal,
  damage,
  energyBuff,
  energyDebuff,
  key
};

enum NpcTemperament {
    passive,
    defensive,
    aggresive
};

enum Usage {
  self,
  other,
  hybrid,
  world
};


class Effect;
class Item;
class Exit;
class Spawn;
class Location;


class Effect {
private:
    EffectType type;
    int amount;

public:

};


class Spawn {
private:
    std::string npc_type;
    uint64_t amount;

public:

};


class Item {
private:
    uint64_t id;
    std::string displayName;
    std::string fullName;
    std::string description;
    bool isObtainable;
    std::vector<Effect> effects;
    Usage usage;

public:

};

class Exit {
private:
    Direction direction;
    uint64_t locationId;
    bool isLocked;
    uint64_t unlockItemId;

public:

};

class Location {
private:
    uint64_t id;
    std::string name;
    std::string description;
    std::vector<Exit> exits;
    std::vector<Spawn> spawns;
    std::vector<uint64_t> itemsIds;

public:
    explicit Location(std::string _name, std::string _desc) : name{_name}, description{_desc} {};

    ~Location() {}

};


class NpcStats {
private:
    uint64_t hp;
    NpcTemperament temperament;

public:

};

class Npc {
private:
    uint64_t id;
    std::string type;
    std::string name;
    std::string description;
    std::vector<std::string> dialogues;
    bool canTrade;
    std::unordered_map<uint64_t, Item> tradeInventory;
    NpcStats stats;

public:

};


class Player {
private:
    uint64_t id;
    std::string name;
    uint64_t hp;
    std::vector<uint64_t> inventory;
    PlayerState state;

public:

};

class World {
private:
    std::vector<Item> items;
    std::vector<Npc> npcs;
    std::vector<Location> locations;
    Location *startLocation;

public:

};
