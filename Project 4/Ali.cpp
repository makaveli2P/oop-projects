//
// Created by dagarw7 on 11/27/2022.
//
#include "Ali.h"
#include <fstream>

Ali::Ali() {
    string fileName, lineRead;
    ifstream inputFile;
    cout << "Enter the SAL file name: ";                // valid inputs for example are test1.txt/test2.txt
    cin >> fileName;
    inputFile.open(fileName);
    if (!inputFile) {
        cout << endl << "No File Read!" << endl;
    }
    while (getline(inputFile, lineRead))
    {
        parsedInput.push_back(lineRead);                // append the read the input file in a vector
    }
    newHard = new Hardware(parsedInput);    // initialize the lone hardware instance
}

void Ali::commandLoop() {
    int j =0;
    string opcode;
    string argValue;
    for (auto item:parsedInput){
        // 2 string from 1 string
        if(item.size()==3){
            opcode = item.substr(0,3);
            argValue = "";
        }
        else{
            opcode = item.substr(0,3);
            argValue = item.substr(4, item.size()-1);
        }

        // create instance of the corresponding Instruction as a Base class pointer dynamically bound to Concrete derived class instances
        if (opcode=="DEC"){
            newHard->programMemory[j] = new DEC(newHard);
        }
        else if(opcode=="LDA"){
            newHard->programMemory[j] = new LDA(newHard);
        }
        else if(opcode=="LDB"){
            newHard->programMemory[j] = new LDB(newHard);
        }
        else if(opcode=="LDI"){
            newHard->programMemory[j] = new LDI(newHard);
        }
        else if(opcode=="STR"){
            newHard->programMemory[j] = new STR(newHard);
        }
        else if(opcode=="XCH"){
            newHard->programMemory[j] = new XCH(newHard);
        }
        else if(opcode=="JMP"){
            newHard->programMemory[j] = new JMP(newHard);
        }
        else if(opcode=="JZS"){
            newHard->programMemory[j] = new JZS(newHard);
        }
        else if(opcode=="JVS"){
            newHard->programMemory[j] = new JVS(newHard);
        }
        else if(opcode=="ADD"){
            newHard->programMemory[j] = new ADD(newHard);
        }
        else if(opcode=="HLT"){
            newHard->programMemory[j] = new HLT(newHard);
        }
        newHard->programMemory[j]->printString = opcode;
        newHard->programMemory[j]->argValue = argValue;
        j++;
    }
    newHard->memLength = parsedInput.size();
    string userInput = "";
    int check = 0;
    while (userInput[0] != 'q')
    {
        cout << "Please select a command:" <<endl;
        cout << "s : Execute a single line of code" << endl;
        cout << "a : Execute all the instructions" << endl;
        cout << "q : Quit the command loop" << endl;
        cout << endl << "User Input: " << endl;
        cin>>userInput;

        if (userInput[0] == 's')
        {
            cout << "The code to be executed is" << endl;
            for (int i =0;i < parsedInput.size();i++){
                newHard->programMemory[i]->print();                         // printing the code using the print method in Instruction
            }
            if(newHard->programCounter < newHard->memLength){

                if(newHard->programMemory[newHard->programCounter]->printString == "DEC"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                    cout << endl << "--------------------------------------------------------------------------"<< endl;
                    cout << "Zero bit = " << newHard->zero_result_bit << ", Overflow bit = " << newHard->overflow_bit << ", RegisterA = " << newHard->registerA << ", RegisterB = " << newHard->registerB << endl;
                    cout << "Program Data" << endl;
                    for (auto pair: newHard->symbolTable) {
                        std::cout << pair.first << " = " << newHard->dataMemory[pair.second] << endl;
                    }
                    cout << "Program Counter at " << newHard->programCounter +1 << ", next instruction executed will be: " << newHard->programMemory[newHard->programCounter]->printString<< " " << newHard->programMemory[newHard->programCounter]->argValue << endl;
                    cout << "--------------------------------------------------------------------------"<< endl;
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "LDA"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                    cout << endl << "--------------------------------------------------------------------------"<< endl;
                    cout << "Zero bit = " << newHard->zero_result_bit << ", Overflow bit = " << newHard->overflow_bit << ", RegisterA = " << newHard->registerA << ", RegisterB = " << newHard->registerB << endl;
                    cout << "Program Data" << endl;
                    for (auto const &pair: newHard->symbolTable) {
                        std::cout << pair.first << " = " << newHard->dataMemory[pair.second] << endl;
                    }
                    cout << "Program Counter at " << newHard->programCounter +1 << ", next instruction executed will be: " << newHard->programMemory[newHard->programCounter]->printString<< " " << newHard->programMemory[newHard->programCounter]->argValue << endl;
                    cout << "--------------------------------------------------------------------------"<< endl;
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "LDB"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                    cout << endl << "--------------------------------------------------------------------------"<< endl;
                    cout << "Zero bit = " << newHard->zero_result_bit << ", Overflow bit = " << newHard->overflow_bit << ", RegisterA = " << newHard->registerA << ", RegisterB = " << newHard->registerB << endl;
                    cout << "Program Data" << endl;
                    for (auto const &pair: newHard->symbolTable) {
                        std::cout << pair.first << " = " << newHard->dataMemory[pair.second] << endl;
                    }
                    cout << "Program Counter at " << newHard->programCounter +1 << ", next instruction executed will be: " << newHard->programMemory[newHard->programCounter]->printString<< " " << newHard->programMemory[newHard->programCounter]->argValue << endl;
                    cout << "--------------------------------------------------------------------------"<< endl;
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "LDI"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                    cout << endl << "--------------------------------------------------------------------------"<< endl;
                    cout << "Zero bit = " << newHard->zero_result_bit << ", Overflow bit = " << newHard->overflow_bit << ", RegisterA = " << newHard->registerA << ", RegisterB = " << newHard->registerB << endl;
                    cout << "Program Data" << endl;
                    for (auto const &pair: newHard->symbolTable) {
                        std::cout << pair.first << " = " << newHard->dataMemory[pair.second]<< endl;
                    }
                    cout << "Program Counter at " << newHard->programCounter +1 << ", next instruction executed will be: " << newHard->programMemory[newHard->programCounter]->printString<< " " << newHard->programMemory[newHard->programCounter]->argValue << endl;
                    cout << "--------------------------------------------------------------------------"<< endl;
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "STR"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                    cout << endl << "--------------------------------------------------------------------------"<< endl;
                    cout << "Zero bit = " << newHard->zero_result_bit << ", Overflow bit = " << newHard->overflow_bit << ", RegisterA = " << newHard->registerA << ", RegisterB = " << newHard->registerB << endl;
                    cout << "Program Data" << endl;
                    for (auto const &pair: newHard->symbolTable) {
                        std::cout << pair.first << " = " << newHard->dataMemory[pair.second] << endl;
                    }
                    cout << "Program Counter at " << newHard->programCounter +1 << ", next instruction executed will be: " << newHard->programMemory[newHard->programCounter]->printString<< " " << newHard->programMemory[newHard->programCounter]->argValue << endl;
                    cout << "--------------------------------------------------------------------------"<< endl;
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "XCH"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                    cout << endl << "--------------------------------------------------------------------------"<< endl;
                    cout << "Zero bit = " << newHard->zero_result_bit << ", Overflow bit = " << newHard->overflow_bit << ", RegisterA = " << newHard->registerA << ", RegisterB = " << newHard->registerB << endl;
                    cout << "Program Data" << endl;
                    for (auto const &pair: newHard->symbolTable) {
                        std::cout << pair.first << " = " << newHard->dataMemory[pair.second] << endl;
                    }
                    cout << "Program Counter at " << newHard->programCounter +1 << ", next instruction executed will be: " << newHard->programMemory[newHard->programCounter]->printString<< " " << newHard->programMemory[newHard->programCounter]->argValue << endl;
                    cout << "--------------------------------------------------------------------------"<< endl;
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "JMP"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                    cout << endl << "--------------------------------------------------------------------------"<< endl;
                    cout << "Zero bit = " << newHard->zero_result_bit << ", Overflow bit = " << newHard->overflow_bit << ", RegisterA = " << newHard->registerA << ", RegisterB = " << newHard->registerB << endl;
                    cout << "Program Data" << endl;
                    for (auto const &pair: newHard->symbolTable) {
                        std::cout << pair.first << " = " << newHard->dataMemory[pair.second] << endl;
                    }
                    cout << "Program Counter at " << newHard->programCounter +1 << ", next instruction executed will be: " << newHard->programMemory[newHard->programCounter]->printString<< " " << newHard->programMemory[newHard->programCounter]->argValue << endl;
                    cout << "--------------------------------------------------------------------------"<< endl;
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "JVS"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                    cout << endl << "--------------------------------------------------------------------------"<< endl;
                    cout << "Zero bit = " << newHard->zero_result_bit << ", Overflow bit = " << newHard->overflow_bit << ", RegisterA = " << newHard->registerA << ", RegisterB = " << newHard->registerB << endl;
                    cout << "Program Data" << endl;
                    for (auto const &pair: newHard->symbolTable) {
                        std::cout << pair.first << " = " << newHard->dataMemory[pair.second] << endl;
                    }
                    cout << "Program Counter at " << newHard->programCounter +1 << ", next instruction executed will be: " << newHard->programMemory[newHard->programCounter]->printString<< " " << newHard->programMemory[newHard->programCounter]->argValue << endl;
                    cout << "--------------------------------------------------------------------------"<< endl;
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "JZS"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                    cout<< endl << "--------------------------------------------------------------------------"<< endl;
                    cout << "Zero bit = " << newHard->zero_result_bit << ", Overflow bit = " << newHard->overflow_bit << ", RegisterA = " << newHard->registerA << ", RegisterB = " << newHard->registerB << endl;
                    cout << "Program Data" << endl;
                    for (auto const &pair: newHard->symbolTable) {
                        std::cout << pair.first << " = " << newHard->dataMemory[pair.second] << endl;
                    }
                    cout << "Program Counter at " << newHard->programCounter +1 << ", next instruction executed will be: " << newHard->programMemory[newHard->programCounter]->printString<< " " << newHard->programMemory[newHard->programCounter]->argValue << endl;
                    cout << "--------------------------------------------------------------------------"<< endl;
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "ADD"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                    cout << endl << "--------------------------------------------------------------------------"<< endl;
                    cout << "Zero bit = " << newHard->zero_result_bit << ", Overflow bit = " << newHard->overflow_bit << ", RegisterA = " << newHard->registerA << ", RegisterB = " << newHard->registerB << endl;
                    cout << "Program Data" << endl;
                    for (auto const &pair: newHard->symbolTable) {
                        std::cout << pair.first << " = " << newHard->dataMemory[pair.second] << endl;
                    }
                    cout << "Program Counter at " << newHard->programCounter +1 << ", next instruction executed will be: " << newHard->programMemory[newHard->programCounter]->printString<< " " << newHard->programMemory[newHard->programCounter]->argValue << endl;
                    cout << "--------------------------------------------------------------------------"<< endl;
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "HLT") {
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                    cout << endl << "--------------------------------------------------------------------------" << endl;
                    cout << "Zero bit = " << newHard->zero_result_bit << ", Overflow bit = " << newHard->overflow_bit
                         << ", RegisterA = " << newHard->registerA << ", RegisterB = " << newHard->registerB << endl;
                    cout << "Program Data" << endl;
                    for (auto const &pair: newHard->symbolTable) {
                        std::cout << pair.first << " = " << newHard->dataMemory[pair.second] << endl;
                    }
                    cout << "--------------------------------------------------------------------------" << endl;
                }
            }
            check++;
        }

        else if (userInput[0] == 'a'){
            cout << "Executing the whole code as printed below" << endl;
            for (int i =0;i < parsedInput.size();i++){
                newHard->programMemory[i]->print();
            }
            cout << endl << "--------------------------------------------------------------------------" <<endl;
            while (newHard->programCounter < newHard->memLength){
                if(newHard->programMemory[newHard->programCounter]->printString == "DEC"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "LDA"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "LDB"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "LDI"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "STR"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "XCH"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "JMP"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "JVS"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "JZS"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "ADD"){
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                }
                else if(newHard->programMemory[newHard->programCounter]->printString == "HLT") {
                    newHard->programMemory[newHard->programCounter]->execute(newHard->programMemory[newHard->programCounter]->argValue);
                    cout <<endl << "--------------------------------------------------------------------------" << endl;
                    cout << "Zero bit = " << newHard->zero_result_bit << ", Overflow bit = " << newHard->overflow_bit
                         << ", RegisterA = " << newHard->registerA << ", RegisterB = " << newHard->registerB << endl;
                    cout << "Program Data" << endl;
                    for (auto const &pair: newHard->symbolTable) {
                        std::cout << pair.first << " = " << newHard->dataMemory[pair.second] << endl;
                    }
                    cout << "--------------------------------------------------------------------------" << endl;
                }
            }
        }
    }
}