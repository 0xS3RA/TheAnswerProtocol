# Change Objects:



List of possible changes:
- Player inventory change (PlayerChange)
- npc inventory change (WorldChange)
- Player hp change (PlayerChange)
- Npc hp change (WorldChange)
- Player position change (WorldChange)
- Door unlock (WorldChange)
- Player state change (in combat, chilling, dead) (PlayerChange)



## WorldChange:

    order of importance:
      - Items
      - npcs
      - locations (everything except the exits)
      - Connections (location id and exits : other locations ids, direction, islocked, unlockitem id)
      - start location

  - WORLDCHANGE|ITEM:01:Heal Potion:Dante's elixir:Found in the earth of hell, this potion will give you all the life you are missing:1:heal:self|ITEM:02:Knowledge Potion:Dragon skin Juice Bottle:With the power of the wise dragon's knowlegde you will begin your next fight with 2 additional energy points:1:energyBuff:self|ITEM:03:Key:Bartender Lost key:He doesn't know where it was, so you might aswell just keep it:1:key:world|NPC:21:guard:jhon:good guy:<Hey what's up:How are you today ?>:1:<20,01:15,02>:<100:defensive>|NPC:22:peasant:gregory:nice dude:.... etc|LOCATION:333:Starting room:Where everything begins:<north:444:0:0>|LOCATION:444:Dark Forest:An obscure place:<west:555:1:03|north:666:1:04|east:777:0:0>|LOCATION:555:etc etc...
  
  

## PlayerChange:



## PlayerMessage:
