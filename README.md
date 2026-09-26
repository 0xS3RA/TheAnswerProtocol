*This project has been created as part of the 42 curriculum by <login1>[, <login2>].*

# The Answer Protocol

## Description

TAP is a shared-world multiplayer text adventure. An authoritative C++20 TCP server
loads a YAML world, owns every mutable resource, and serves line-oriented RFC 42TAP
commands. The CLI is a thin raw-protocol client that prints asynchronous events while
the user is typing.

This milestone contains the complete server and CLI. The mandatory GUI is deliberately
not implemented yet.

## Instructions

Requirements: CMake 3.20+, a C++20 compiler, GNU Make, and Git. `yaml-cpp` is found
from the system or fetched automatically by CMake. `clang-format` is optional; when it
is absent, `make lint` still performs the strict warnings-as-errors build.

```sh
make build
make run-server
make run-client NAME=alice
make lint
make test
make clean
```

The default endpoint is `127.0.0.1:4242`. Override it with `HOST` and `PORT`; override
the world with `WORLD`.

## Architecture

The server uses a dispatcher and a single-lock authoritative game model:

- Each accepted TCP connection has one blocking reader and one dedicated writer.
- Readers frame the TCP byte stream into LF-terminated lines, so fragmented and
  coalesced packets are handled correctly.
- Outgoing messages enter a bounded queue. A slow or broken client therefore cannot
  block broadcasts to other players.
- `Game` owns all mutable rooms, players, groups, combat, items, and quest progress.
  One mutex serializes command handlers and makes compound mutations atomic.
- `World` owns immutable definitions loaded from YAML and validates every reference
  before the listener starts.

Source layout:

```text
common/Protocol.*  command parsing, RFC formatting, JSON string escaping
common/Socket.*    TCP line reader, writer queue, connection helper
common/Logger.*    structured JSON logging
server/World.*     YAML definitions and referential validation
server/Game.*      authoritative state and command dispatcher
server/server.cpp  listener, CONNECT state machine, connection lifecycle
cli/cli.cpp        asynchronous raw-protocol terminal client
```

## Protocol Implementation

The wire format follows RFC 42TAP: UTF-8 text over TCP, one message per LF-terminated
line. The server greets clients with `OK hello proto=1`; clients then send
`CONNECT <username>`.

Implemented RFC commands:

`CONNECT`, `LOOK`, `MOVE`, `QUIT`, `CHAT`, `WHO`, `GROUP CREATE`, `GROUP INVITE`,
`GROUP JOIN`, `GROUP LEAVE`, `TAKE`, `DROP`, `INVENTORY`, `TALK`, `ATTACK`, `STATUS`,
`QUEST`, and `QUESTS`.

Documented extensions and choices:

- `DEFEND` and `FLEE` implement the extra combat actions requested by the subject.
- Underspecified malformed input uses `ERR 400 BAD_REQUEST`; pre-authenticated game
  commands use `ERR 202 NOT_CONNECTED`; repeated CONNECT uses
  `ERR 203 ALREADY_CONNECTED`; invalid DEFEND/FLEE uses `ERR 407 NOT_IN_COMBAT`;
  missing targets use `ERR 408 TARGET_MISSING`.
- Group lookup extensions reuse `404` with explicit `PLAYER_NOT_FOUND` or
  `GROUP_INVITE_NOT_FOUND`; a non-leader invite uses `401 NOT_GROUP_LEADER`.
- `WHO` follows the RFC response `OK players=<count>`.
- `TALK` follows the RFC plain-dialogue response.
- Additional informational events are `EVT ROOM ITEM ...`, `EVT ROOM COMBAT ...`,
  `EVT QUEST OBJECTIVE ...`, and `EVT QUEST COMPLETE ...`. Clients may ignore
  unknown events.
- Resource matching is case-insensitive and accepts an instance ID, definition ID,
  short ID suffix, or complete display name.
- Disconnecting drops all held unique item instances into the player's last room.

The maximum input line is 1024 bytes. Control characters are rejected. A queue holds
at most 256 pending messages per connection.

## Combat System

Players start with 100 HP. An `ATTACK <npc>` turn works as follows:

1. The player attacks first for 12 damage plus all inventory attack bonuses.
2. A surviving enemy immediately counters for its configured attack value.
3. `DEFEND` prepares a guard that halves the next counterattack.
4. `FLEE` clears the current combat without dealing damage.

Enemy types have different HP and attack values. Dead NPC instances are removed from
their room. At zero HP, a player respawns in `loc.square` with 50 HP. Moving also ends
the current combat. The Iron Sword quest reward adds 5 damage.

## Quest System

`QUEST <npc>` starts that NPC's quest. Calling it again at the giver reports progress
or turns the quest in when ready:

- Fetch objectives count matching unique instances in the player's inventory. Turn-in
  consumes the requested items.
- Defeat objectives advance when the player kills the target NPC type.
- Completion creates a new unique reward instance and emits
  `EVT QUEST COMPLETE <quest-id>`.
- Completed quests cannot be accepted again. `QUESTS` lists active and completed
  progress.
- Consumed fetch resources respawn at their original world spawn, and defeated hostile
  NPCs respawn as new instances. This keeps every personal quest completable in a
  long-running multiplayer server without duplicating an existing instance.

The world includes `quest.herbs` (fetch Healing Herbs) and `quest.goblin` (defeat the
Cave Goblin).

## World Design

`world.yaml` contains ten rooms. The square/bakery/north-gate/market circuit and the
north-gate/watchtower/forest-path/market circuit provide loops; tavern, well, clearing,
and crypt provide branches.

NPC roles include dialogue NPCs, quest-givers, and hostile enemies. Six item types are
defined, four of which spawn as obtainable resources. Every spawned item and NPC is a
runtime instance with its own ID.

The loader validates room exits, item and NPC spawns, quest givers, targets, rewards,
minimum world sizes, combat stats, and obtainable item counts.

## Server Logging

The server writes one JSON object per line to standard error through an asynchronous
logger. Its lossless queue preserves every required entry. Every record has a UTC
timestamp, level (`INFO`, `WARN`, or `ERROR`), event name, and structured fields. Disk
or pipe latency never occurs while the game-state mutex is held.

Events cover connection lifecycle and remote address, commands and replies, errors,
movement, item transfers, chat, group changes, combat, respawn, and quest progression.
More than 20 commands from one connection in one second emits an
`abuse_command_flood` warning. Redirect logs with:

```sh
make run-server 2>server.log
```

## Building and Running

Direct commands, equivalent to the Make targets:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/bin/tap-server --host 127.0.0.1 --port 4242 --world world.yaml
./build/bin/tap-cli --host 127.0.0.1 --port 4242 --name alice
```

Without `--name`, type `CONNECT alice` manually. The CLI sends every following line
unchanged, making it suitable for protocol testing.

`make run-client-gui` currently reports that the GUI remains to be implemented.

## Testing

`make test` builds and runs CTest coverage for command parsing, JSON escaping, world
validation, movement, errors, unique item transfer, multi-word item names, groups,
combat, and defeat-quest completion.

Manual multiplayer test:

1. Run the server and two CLI instances with different names.
2. Test `LOOK`, room/global/group chat, movement, `WHO`, and presence events.
3. Take an item in one client and verify it disappears for the other.
4. Accept the guard quest, reach the crypt, defeat the goblin, return, and turn it in.
5. Close a client abruptly and verify leave/stats events and JSON logs.

## Group Contributions

- `<login1>`: server, protocol, networking, tests, and CLI.
- `<login2>`: world design and future GUI.

Replace these placeholders with the real group logins before submission.

## Resources

- RFC 42TAP supplied with the subject (`protocol-rfc.html`)
- RFC 2119, requirement language
- RFC 5234, ABNF
- RFC 793, TCP
- RFC 3629, UTF-8
- CMake and yaml-cpp documentation

AI was used to audit the initial non-compliant Protobuf transport, compare it with the
subject and RFC, design the replacement architecture, assist with implementation, and
review build/test failures. Every team member remains responsible for understanding and
validating the submitted code.
