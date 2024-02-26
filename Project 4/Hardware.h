//
// Created by dagarw7 on 11/23/2022.
//
#include <vector>
#include <iostream>
#include <string>
#include <map>
#include "Instruction.h"

using namespace std;

#ifndef PROJECT4_HARDWARE_H
#define PROJECT4_HARDWARE_H

class Hardware {

public:
    Instruction *programMemory[128];        // program memory array of length 128, of Instruction type
    int dataMemory[128];                    // data memory array of length 128, of int type
    map<string, int> symbolTable;           // symbol table hashmap to store the symbols
    int registerA;                          // accumulator or Register A
    int registerB;                          // Register B integer variable
    int programCounter;                     // program counter variable to hold the address of the next instruction to be executed
    int zero_result_bit;                    // zero result bit to be set if ADD instruction produced a zero result
    int overflow_bit;                       // overflow bit to be set if ADD instruction produces result more than 2^31
    int DM_pointer;                         // data memory pointer that points to the start of the data memory part from 0 index
    int memLength;                          // variable to store the length of program memory till where the program is stored

    Hardware(vector<string> instructionVec);

    ~Hardware();

};
#endif //PROJECT4_HARDWARE_H
