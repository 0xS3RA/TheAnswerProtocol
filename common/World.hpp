#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>


enum commandType {
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

enum Effect_Type {
    heal,
    damage
};

enum NpcTemperament {
    passive,
    defensive,
    aggresive
};


class Effect;
class Item;
class Exit;
class Spawn;
class Location;


class Item {
private:
    std::string displayName;
    std::string fullName;
    std::string description;
    bool isObtainable;
    std::vector<Effect> effects;

public:

};

class Exit {
private:
    Direction direction;
    Location *location;
    bool isLocked;
    Item unlockItem;

public:

};

class Location {
private:
    uint64_t id;
    std::string name;
    std::string description;
    std::vector<Exit> exits;
    std::vector<Spawn> spawns;
    std::vector<Item> items;

public:
    explicit Location(std::string _name, std::string _desc) : name{_name}, description{_desc} {};

    ~Location() {}

};


class Spawn {
private:
    std::string npc_type;
    uint64_t amount;

public:

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

class Effect {
private:
    Effect_Type type;
    int amount;

public:

};

class Player {
private:
    uint64_t id;
    std::string name;
    uint64_t hp;
    std::vector<Item> inventory;
    PlayerState state;

public:

};

class World {
private:
    std::vector<Npc> npcs;
    std::vector<Location> locations;
    Location *startLocation;

public:

};
