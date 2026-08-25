#pragma once

// make base class Command
// make class MessageCommand
// make class InteractionCommand

class Command {
    private:

    public:
        virtual void test() const {
            std::cout << "generic test" << std::endl;
        }

        virtual ~Command() = default;
};



class MessageCommand : public Command {
    private:


    public:
        void test() const override {
            std::cout << "Message test" << std::endl;
        }
};




class InteractionCommand : public Command {
    private:


    public:
        void test() const override {
            std::cout << "Interaction test" << std::endl;
        }
};
