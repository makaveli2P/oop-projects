//
// Created by dagarw7 on 11/23/2022.
//
#include <string>

#ifndef PROJECT4_INSTRUCTION_H
#define PROJECT4_INSTRUCTION_H

class Hardware;
class Instruction{
protected:
    Hardware* instHard;                                 // Hardware type data member to be passed as reference
public:
    virtual void execute(std::string argument) = 0;     // pure virtual member function
    std::string printString;            // opcode data member
    std::string argValue;               // argvalue data member

    void print();                       // member function for print

    Instruction();                      // default constructor

    Instruction(std::string i);         // parameterized constructor

    Instruction(Hardware* someHardware);    // parameterized constructor

    virtual ~Instruction() = default;       // virtual destructor

};

#endif //PROJECT4_INSTRUCTION_H
