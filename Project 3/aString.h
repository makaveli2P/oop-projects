//
// Created by dagarw7 on 11/10/2022.
//

#ifndef PROJECT3_ASTRING_H
#define PROJECT3_ASTRING_H

class aString{

public:
    int size_;
    char* chars_;

    aString();                      // default constructor

    aString(char* val);             //convenience/parameterized constructor

    aString(const aString& value);  // copy constructor

    ~aString();
};

#endif //PROJECT3_ASTRING_H


