#include "Command.hpp"
#include "World.hpp"



CommandType Command::get_command_type(const std::string_view unparsedCommand) {
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



std::optional<InteractionCommand>
InteractionCommand::parse(const std::string_view fullCommand) {

    // Exemple command : INTERACTION:ACCEPTQUEST:5489

    int firstPos{0};
    int cSize {0};
    std::string extractedArg {};
    InteractionType firstArg {};
    uint64_t secondArg {};

    while (fullCommand[firstPos] != ':')
        firstPos++;

    firstPos += 1;

    while (fullCommand[firstPos + cSize] != ':') {
        cSize++;
    }
    extractedArg = fullCommand.substr(firstPos, cSize);

    if (extractedArg == "ACCEPTQUEST")
        firstArg = acceptQuest;
    else if (extractedArg == "TRADE")
        firstArg = trade;
    else if (extractedArg == "ATTACK")
        firstArg = attack;
    else if (extractedArg == "SPEAK")
        firstArg = speak;
    else if (extractedArg == "OPEN")
        firstArg = open;
    else if (extractedArg == "DROP")
        firstArg = drop;
    else if (extractedArg == "PICKUP")
        firstArg = pickUp;
    else if (extractedArg == "MOVE")
        firstArg = move;
    else
        return std::nullopt;

    firstPos += cSize + 1;
    cSize = 0;
    while (fullCommand[firstPos + cSize] != '\n') {
        cSize++;
    }
    extractedArg = fullCommand.substr(firstPos, cSize);
    secondArg = std::stoi(extractedArg);
    if (secondArg == 0)
        return std::nullopt;

    return InteractionCommand(firstArg, secondArg);

}



std::optional<AttackCommand>
AttackCommand::parse(const std::string_view fullCommand) {

    // Exemple command : ATTACK:CONCENTRATE

    int firstPos{0};
    int cSize {0};
    std::string extractedArg {};
    CombatAction firstArg {};

    while (fullCommand[firstPos] != ':')
        firstPos++;

    firstPos += 1;

    while (fullCommand[firstPos + cSize] != '\n') {
        cSize++;
    }
    extractedArg = fullCommand.substr(firstPos, cSize);

    if (extractedArg == "LIGHT")
        firstArg = lightAttack;
    else if (extractedArg == "STRONG")
        firstArg = strongAttack;
    else if (extractedArg == "CONCENTRATE")
        firstArg = concentrate;
    else if (extractedArg == "FLEE")
        firstArg = flee;
    else if (extractedArg == "HALF")
        firstArg = halfDefend;
    else if (extractedArg == "FULL")
        firstArg = fullDefend;
    else if (extractedArg == "COUNTER")
        firstArg = counter;
    else if (extractedArg == "NONE") {
        firstArg = none;
    } else
        return std::nullopt;

    return AttackCommand(firstArg);
}



std::optional<MessageCommand>
MessageCommand::parse(const std::string_view fullCommand) {

    // Exemple command : MESSAGE:Bonsoir paris!

    int firstPos{0};
    int cSize {0};
    std::string extractedArg {};

    while (fullCommand[firstPos] != ':')
        firstPos++;

    firstPos += 1;

    while (fullCommand[firstPos + cSize] != '\n') {
        cSize++;
    }
    extractedArg = fullCommand.substr(firstPos, cSize);
    if (!extractedArg.empty())
        return MessageCommand(extractedArg);
    else
        return std::nullopt;
}
