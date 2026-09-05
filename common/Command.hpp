#pragma once

#include <cstdint>
#include <optional>
#include <variant>

#include "template.pb.h"



class InteractionCommand;
class MessageCommand;
class AttackCommand;


using CommandVariant = std::variant<InteractionCommand, AttackCommand, MessageCommand>;
using ChangeVariant = std::variant<WorldChange, PlayerChange, PlayerMessage>




class Command {
public:
    virtual ~Command() = default;
    static game::CommandType get_command_type(const std::string_view unparsedCommand);

};




class InteractionCommand : public Command {
private:
    game::InteractionType type;
    uint64_t targetid;
public:
    InteractionCommand(game::InteractionType type_, uint64_t id_) : type{type_}, targetid{id_} {}
    static std::optional<InteractionCommand> parse(const std::string_view fullcommand);
};





class AttackCommand : public Command {
private:
    game::CombatAction action;
public:
    AttackCommand(game::CombatAction action_) : action{action_} {}
    static std::optional<AttackCommand> parse(const std::string_view fullcommand);
};




class MessageCommand : public Command {
private:
    std::string message;
public:
    MessageCommand(std::string message_) : message {message_} {}
    static std::optional<MessageCommand> parse(const std::string_view fullcommand);

};
