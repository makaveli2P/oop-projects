//
// Created by dagarw7 on 11/23/2022.
//
#include <iostream>
#include "Instruction.h"

using namespace std;
Instruction::Instruction(std::string i) {               // split the i argument string
    if(i.size()==3){
        printString = i.substr(0,3);
        argValue = "";
    }else{
        printString = i.substr(0,3);
        argValue = i.substr(4, i.size()-1);
    }
}

void Instruction::print(){
    if (!printString.empty())
        cout <<printString << " " << argValue << endl;
}

Instruction::Instruction(Hardware* someHardware) {
    instHard = someHardware;
}

Instruction::Instruction() {
    printString = "";
    argValue = "";
}



