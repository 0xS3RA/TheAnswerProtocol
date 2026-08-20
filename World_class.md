
**Enums:**
direction : north, east, west, south
effect_type : heal, damage (pour le moment)
npcTemperament: passive, defensive, aggresive

**class location:**
- name (std::string)
- description (std::string)
- exits (std::vector<instances exit;>)
- spawns (std::vector<instances spawn;>)
- items (std::vector<instances item;>)


**class exit:**
- direction (enum direction)
- location (instance location)
- isLocked (bool)
- unlockItem (instance Item)


**class spawn:**
- npc_type (std::string)
- amount (uint64_t)


**class item:**
- displayName (std::string)
- fullName (std::string)
- description (std::string)
- isObtainable (bool)
- Effects (std::vector<instances effect;>)


**class NPC:**
- type (std::string)
- name (std::string)
- description (std::string)
- dialogues (std::vector<std::string>)
- canTrade (bool)
- tradeInventory (std::unordered_map<uint64_t, item>)
- stats (instance npcStats)


**class effect:**
- type (enum effect_type)
- amount (int)


**class npcStats:**
- hp (uint64_t)
- Temperament (enum npcTemperament)


**class player:**
- Id (uint64_t)
- Name (std::string)
- hp (uint32_t)
- inventory (std::vector<&item;>)
- 


**class world:**
- items (std::vector<item;>)
- npcs (std::vector<npc;>)
- locations (std::vector<location;>)
- startLocation (instance &location)
- 