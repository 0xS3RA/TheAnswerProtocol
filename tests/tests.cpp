#include "common/Protocol.hpp"
#include "server/Game.hpp"
#include "server/World.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace {

class FakeSink final : public tap::MessageSink {
public:
    bool enqueue(std::string message) override
    {
        messages.push_back(std::move(message));
        return true;
    }

    std::vector<std::string> messages;
};

void require(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

tap::CommandResult run(tap::Game& game, const std::string& player, const std::string& command)
{
    return game.handle(player, tap::parse_command(command));
}

void test_protocol()
{
    const tap::Command command = tap::parse_command("chat room hello shared world");
    require(command.name == "CHAT", "command names are case-insensitive");
    require(command.rest(1) == "hello shared world", "free-form arguments are preserved");
    require(tap::ok("connected") == "OK connected", "OK response formatting");
    require(tap::error(301, "NO_EXIT") == "ERR 301 NO_EXIT", "error formatting");
    require(tap::json_string("a\"b") == "\"a\\\"b\"", "JSON escaping");
    require(tap::is_safe_text("héros"), "valid UTF-8 is accepted");
    require(!tap::is_safe_text(std::string("\xc0\xaf", 2)), "invalid UTF-8 is rejected");
}

void test_line_framing()
{
    int sockets[2]{};
    require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "socketpair creation");
    const std::string payload = std::string(1100, 'x') + "\nLOOK\n";
    require(::send(sockets[0], payload.data(), payload.size(), 0) ==
                static_cast<ssize_t>(payload.size()),
            "test payload send");

    tap::LineReader reader(sockets[1]);
    std::string line;
    require(reader.read(line) == tap::ReadStatus::TooLong, "oversized line rejection");
    require(reader.read(line) == tap::ReadStatus::Line && line == "LOOK",
            "framing recovers after oversized line");
    ::close(sockets[0]);
    ::close(sockets[1]);
}

void test_world()
{
    const tap::World world = tap::World::load(TAP_WORLD_PATH);
    require(world.rooms.size() >= 8, "world room minimum");
    require(world.items.size() >= 4, "world item minimum");
    require(world.quests.size() >= 2, "world quest minimum");
}

void test_items_and_movement()
{
    tap::Logger logger;
    tap::Game game(tap::World::load(TAP_WORLD_PATH), logger);
    auto alice = std::make_shared<FakeSink>();
    require(game.connect("alice", alice) == "OK connected", "player connects");
    require(run(game, "alice", "MOVE north").reply == "OK room=loc.bakery", "valid movement");
    require(run(game, "alice", "MOVE north").reply == "ERR 301 NO_EXIT", "invalid movement");

    const std::string taken = run(game, "alice", "TAKE Loaf of Bread").reply;
    require(taken.starts_with("OK taken=item.bread."), "multi-word TAKE");
    require(run(game, "alice", "TAKE Loaf of Bread").reply == "ERR 404 ITEM_NOT_FOUND",
            "taken item leaves room");
    require(run(game, "alice", "INVENTORY").reply.find("item.bread.") != std::string::npos,
            "inventory contains unique item instance");
    game.disconnect("alice");
}

void test_groups()
{
    tap::Logger logger;
    tap::Game game(tap::World::load(TAP_WORLD_PATH), logger);
    auto alice = std::make_shared<FakeSink>();
    auto bob = std::make_shared<FakeSink>();
    require(game.connect("alice", alice) == "OK connected", "group leader connects");
    require(game.connect("bob", bob) == "OK connected", "group member connects");
    require(run(game, "alice", "GROUP CREATE").reply == "OK group=group.1", "group creation");
    require(run(game, "alice", "GROUP INVITE bob").reply == "OK", "group invitation");
    require(run(game, "bob", "GROUP JOIN alice").reply == "OK group=group.1", "group join");
    require(run(game, "bob", "CHAT GROUP hello").reply == "OK", "group chat");
}

void test_combat_quest()
{
    tap::Logger logger;
    tap::Game game(tap::World::load(TAP_WORLD_PATH), logger);
    auto alice = std::make_shared<FakeSink>();
    require(game.connect("alice", alice) == "OK connected", "quest player connects");

    run(game, "alice", "MOVE north");
    run(game, "alice", "MOVE east");
    require(run(game, "alice", "QUEST Village Guard").reply.find("\"status\":\"active\"") !=
                std::string::npos,
            "defeat quest acceptance");
    run(game, "alice", "MOVE east");
    run(game, "alice", "MOVE south");
    run(game, "alice", "MOVE south");
    run(game, "alice", "MOVE down");
    run(game, "alice", "ATTACK Cave Goblin");
    run(game, "alice", "ATTACK Cave Goblin");
    require(run(game, "alice", "ATTACK Cave Goblin").reply.find("\"status\":\"victory\"") !=
                std::string::npos,
            "enemy defeat");
    run(game, "alice", "MOVE up");
    run(game, "alice", "MOVE north");
    run(game, "alice", "MOVE north");
    run(game, "alice", "MOVE west");
    require(run(game, "alice", "QUEST guard").reply.find("\"status\":\"completed\"") !=
                std::string::npos,
            "quest completion and turn-in");

    auto bob = std::make_shared<FakeSink>();
    require(game.connect("bob", bob) == "OK connected", "second quest player connects");
    run(game, "bob", "MOVE north");
    run(game, "bob", "MOVE east");
    run(game, "bob", "QUEST guard");
    run(game, "bob", "MOVE east");
    run(game, "bob", "MOVE south");
    run(game, "bob", "MOVE south");
    run(game, "bob", "MOVE down");
    require(run(game, "bob", "ATTACK goblin").reply.starts_with("OK "),
            "defeated quest target respawns for other players");
}

void test_fetch_quest()
{
    tap::Logger logger;
    tap::Game game(tap::World::load(TAP_WORLD_PATH), logger);
    auto alice = std::make_shared<FakeSink>();
    require(game.connect("alice", alice) == "OK connected", "fetch player connects");
    require(run(game, "alice", "QUEST Village Merchant").reply.find("\"status\":\"active\"") !=
                std::string::npos,
            "fetch quest acceptance");
    run(game, "alice", "MOVE east");
    run(game, "alice", "MOVE east");
    run(game, "alice", "MOVE south");
    require(run(game, "alice", "TAKE Healing Herbs").reply.starts_with("OK taken=item.herbs."),
            "fetch objective item");
    run(game, "alice", "MOVE north");
    run(game, "alice", "MOVE west");
    run(game, "alice", "MOVE west");
    require(run(game, "alice", "QUEST merchant").reply.find("\"status\":\"completed\"") !=
                std::string::npos,
            "fetch quest turn-in");
    require(run(game, "alice", "INVENTORY").reply.find("item.coin.") != std::string::npos,
            "fetch quest reward");

    auto bob = std::make_shared<FakeSink>();
    require(game.connect("bob", bob) == "OK connected", "second fetch player connects");
    run(game, "bob", "MOVE east");
    run(game, "bob", "MOVE east");
    run(game, "bob", "MOVE south");
    require(run(game, "bob", "TAKE herbs").reply.starts_with("OK taken=item.herbs."),
            "consumed fetch resource respawns for other players");
}

} // namespace

int main()
{
    test_protocol();
    test_line_framing();
    test_world();
    test_items_and_movement();
    test_groups();
    test_combat_quest();
    test_fetch_quest();
    std::cout << "All TAP tests passed\n";
}
