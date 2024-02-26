//
// Created by dagarw7 on 11/27/2022.
//
#include "Subclass.h"

DEC::DEC(Hardware* someHardware) {
    instHard = someHardware;
}

void DEC::execute(string argument) {
    instHard->symbolTable[argument] = instHard->DM_pointer;             // creates a new key in the symbol table hash for the symbol associated with DEC and assigns a valid memory location
    instHard->DM_pointer += 1;
    instHard->programCounter += 1;

}

LDA::LDA(Hardware *someHardware) {
    instHard = someHardware;
}

void LDA::execute(string argument) {
    auto val = instHard->symbolTable.find(argument);
    instHard->registerA = instHard->dataMemory[val->second];            // takes the value of the symbol argument from the symbol table hash and loads it in register A
    instHard->programCounter += 1;
}

LDB::LDB(Hardware *someHardware) {
    instHard = someHardware;
}

void LDB::execute(string argument) {
    auto val = instHard->symbolTable.find(argument);
    instHard->registerB = instHard->dataMemory[val->second];            // takes the value of the symbol argument from the symbol table hash and loads it in register B
    instHard->programCounter += 1;
}

LDI::LDI(Hardware *someHardware) {
    instHard = someHardware;
}

void LDI::execute(string argument) {
    instHard->registerA = stoi(argument);                       // loads the integer value as per the argument into register A
    instHard->programCounter += 1;
}

STR::STR(Hardware *someHardware) {
    instHard = someHardware;
}

void STR::execute(string argument) {
    instHard->dataMemory[instHard->symbolTable[argument]] = instHard->registerA;            // stores the value in register A to the corresponding symbol in symbol table hash
    instHard->programCounter += 1;
}

XCH::XCH(Hardware *someHardware) {
    instHard = someHardware;
}

void XCH::execute(string argument) {
    swap(instHard->registerA, instHard->registerB);             // exchanges the values of register A and register B through in built swap method
    instHard->programCounter += 1;
}

JMP::JMP(Hardware *someHardware) {
    instHard = someHardware;
}

void JMP::execute(string argument) {
    instHard->programCounter = stoi(argument);              // # sets the program counter value as per the argument in the source code
}

JZS::JZS(Hardware *someHardware) {
    instHard = someHardware;
}

void JZS::execute(string argument) {
    if (instHard->zero_result_bit == 1){                    // if conditional to check if the zero result bit is set or not
        instHard->programCounter = stoi(argument);
    }
    else{
        instHard->programCounter += 1;                      // if not set, increment the program counter continue execution as per source code sequence
    }
}

JVS::JVS(Hardware *someHardware) {
    instHard = someHardware;
}

void JVS::execute(string argument) {
//    cout << "yt"
    if (instHard->overflow_bit == 1){                       // if conditional to check if the overflow bit is set or not
        instHard->programCounter = stoi(argument);
    }
    else{
        instHard->programCounter += 1;                      // if not set, increment the program counter and continue execution as per source code sequence
    }
}

ADD::ADD(Hardware *someHardware) {
    instHard = someHardware;
}

void ADD::execute(string argument) {
    long long int tempSum {0};
    long long int tempA = instHard->registerA;
    long long int tempB = instHard->registerB;
    tempSum = tempA + tempB;

    int sum {0};
    sum = instHard->registerA + instHard->registerB;            // computing the sum of register A and register B
    instHard->registerA = sum;                                  // storing the result in register A

    if (sum == 0){
        instHard->zero_result_bit = 1;                          // setting the zero result bit as 1 if the computed sum equals zero
    }
    else{
        instHard->zero_result_bit = 0;                          // clearing the zero result and overflow bit if the add instruction produces a legal result
    }
    if ((tempSum > 2147483647) || (tempSum < -2147483647)){
        instHard->overflow_bit = 1;                             // setting the overflow bit as 1 if the computed sum is greater than 2147483647 ( Equivalent 2's complement 32-bit representation: 0111 1111 1111 1111 1111 1111 1111 1111)
//        instHard->registerA = tempSum - 2147483647;             // storing the overflow value in register A
    }
    else{
        instHard->overflow_bit = 0;                             // clearing the overflow bit if the add instruction produces a legal result
    }
    instHard->programCounter += 1;                              // increments the program counter in the end
}

HLT::HLT(Hardware *someHardware) {
    instHard = someHardware;
}

void HLT::execute(string argument) {
    instHard->programCounter = instHard->memLength + 10;
}
