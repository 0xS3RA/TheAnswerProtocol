#pragma once

#include <cstdint>
#include <iostream>

#include "World.hpp"


class Command {
private:

public:
    ~Command() = default;

    static commandType get_command_type(const std::string_view unparsedCommand) {
        std::string sub {};
        ssize_t i{0};
        while (unparsedCommand[i] != ':')
            i++;
        sub = unparsedCommand.substr(0, i);
        if (sub == "INTERACTION")
            return Interaction;
        else if (sub == "ATTACK")
            return Attack;
        else if (sub == "MESSAGE")
            return Message;
        else { return Invalid; }
    }

    static InteractionCommand parseInteractionCommand(const std::string_view fullCommand) {
        int firstPos{0};
        int cSize {0};
        std::string firsArg {};
        std::string secondArg {};

        while (fullCommand[firstPos] != ':')
            firstPos++;

        firstPos += 1;

        for (int j{0}; j < 2; j++) {
            while (fullCommand[firstPos + cSize] != ':') {
                cSize++;
            }
            firstArg = fullCommand.substr(firstPos, cSize);



        }
    }

    static AttackCommand parseAttackCommand(std::string_view fullCommand) {


    }

    static MessageCommand parseMessageCommand(std::string_view fullCommand) {


    }

};



class InteractionCommand : public Command {
private:
    InteractionType type;
    uint64_t targetId;


public:
    InteractionCommand(InteractionType type_, uint64_t id_) : type{type_}, targetId{id_} {}
};




class AttackCommand : public Command {
private:
    CombatAction action;

};




class MessageCommand : public Command {
private:
    std::string message;

};
