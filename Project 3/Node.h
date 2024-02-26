//
// Created by dagarw7 on 11/10/2022.
//

#ifndef PROJECT3_NODE_H
#define PROJECT3_NODE_H

#include "aString.h"

class Node{

public:
    aString* data;
    Node* left ;
    Node* right;

    Node();                         // default constructor

    Node(aString value);            // parameterized constructor

    Node(const Node& curr_node);    // copy constructor

    ~Node();                        // destructor
};

#endif //PROJECT3_NODE_H
