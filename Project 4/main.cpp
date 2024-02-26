// Author  :   Dhruv Agarwal (UIN: 651945114)
// Date    :   December 02, 2022
// Content :   Assembly Language Interpreter (ALI) for a Simple Assembly Language (SAL)
//
// DESIGN - I have followed two principles : a) Derived class specialized Base class ,
//                                           b) Inheritance of Base class objects by derived class.
// Based on these principles, I have 3 Main Classes: a) Hardware - implements memory and related hardware, this is passed as reference in the Instruction Base Class
//                                                   b) Instruction - Base class for asntract class instructions
//                                                   d) Subclass - derived class for all the derived classes
//                                                   c) ALI - Interpreter that executes user command.
// NOTE: All instructions like DEC, STR, etc are implemented as instances of instruction base class dynamically bound to the specific instruction instance
//
// Inheritance: Command subclasses DEC, STR,etc inherit data members and pure virtual function execute from Base class Instruction
// I refine the execute function in each sub-class and implement specialized instructions
//
// Flow: ALI class creates Hardware as instances ---> takes user input ---> executes instructions by using Instruction object
// --->  all instruction object uses same hardware instance
//NOTE: In Clion IDE the text files to be read need to be in the debug folder and not the main directory

#include <fstream>
#include "Hardware.h"
#include "Instruction.h"
#include "Ali.h"
using namespace std;

int main() {

    Ali new_ali;                // create a new instance of the ALI class
    new_ali.commandLoop();      // call the command loop on that instance

    return 0;
}