//
// Created by dagarw7 on 11/27/2022.
//
#include "Hardware.h"

#ifndef PROJECT4_DEC_H
#define PROJECT4_DEC_H

class DEC : public Instruction {
public:
    DEC(Hardware* someHardware);
    void execute(string argument) override;

};

class LDA: public Instruction{
public:
    LDA(Hardware* someHardware);
    void execute(string argument);
};

class LDB: public Instruction{
public:
    LDB(Hardware* someHardware);
    void execute(string argument);
};

class LDI: public Instruction{
public:
    LDI(Hardware* someHardware);
    void execute(string argument);
};

class STR: public Instruction{
public:
    STR(Hardware* someHardware);
    void execute(string argument);
};

class XCH: public Instruction{
public:
    XCH(Hardware* someHardware);
    void execute(string argument);
};

class JMP: public Instruction{
public:
    JMP(Hardware* someHardware);
    void execute(string argument);
};

class JZS: public Instruction{
public:
    JZS(Hardware* someHardware);
    void execute(string argument);
};

class JVS: public Instruction{
public:
    JVS(Hardware* someHardware);
    void execute(string argument);
};

class ADD: public Instruction{
public:
    ADD(Hardware* someHardware);
    void execute(string argument);
};

class HLT: public Instruction{
public:
    HLT(Hardware* someHardware);
    void execute(string argument);
};

#endif //PROJECT4_DEC_H
