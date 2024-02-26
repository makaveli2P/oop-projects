//
// Created by dagarw7 on 11/27/2022.
//
#include "Hardware.h"
#include "Subclass.h"

#ifndef PROJECT4_ALI_H
#define PROJECT4_ALI_H

class Ali{
public:
    vector<string> parsedInput;
    Hardware* newHard;
    Ali();
    void commandLoop();
};

#endif //PROJECT4_ALI_H
