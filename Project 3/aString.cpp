//
// Created by dagarw7 on 11/10/2022.
//

#include <iostream>
#include <string.h>
#include "aString.h"

using namespace std;

aString::aString(){                                 // default constructor
    size_ = 0;
    chars_ = new char[size_ + 1];
}

aString::aString(char* val) {                       //convenience/parameterized constructor
    if (val == nullptr){
        chars_ = new char[size_];
        size_ = 0;
    }
    else{
        size_ = strlen((val));
        chars_ = new char[size_ + 1];
        strcpy(chars_,val);
    }
}

aString::aString(const aString& value) {             // copy constructor
    size_ = strlen(value.chars_);
    chars_ = new char[size_ + 1];
    strcpy(chars_,value.chars_);
}

aString::~aString(){                                 // destructor
    cout << chars_;
    delete[] chars_;
}

