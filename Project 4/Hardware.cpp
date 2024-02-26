//
// Created by dagarw7 on 11/23/2022.
//
#include "Hardware.h"

Hardware::Hardware(vector<string> instructionVec) {
    registerA = 0;
    registerB = 0;
    programCounter = 0;
    memLength = 0;
    zero_result_bit = 0;
    overflow_bit = 0;
    dataMemory[128] = {};
    DM_pointer = dataMemory[0];
}

Hardware::~Hardware() {
}
