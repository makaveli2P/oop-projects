//
// Created by dagarw7 on 11/10/2022.
//

#include "Node.h"
#include <iostream>
#include <string.h>

Node::Node(){
    data = new aString();
    left = nullptr;
    right = nullptr;
}

Node::Node(aString value){
    data = new aString(value);
    left = nullptr;
    right = nullptr;
}

Node::Node(const Node& curr_node){
    data = new aString(curr_node.data->chars_);
    left = nullptr;
    right = nullptr;
}

Node::~Node(){
    data->~aString();
//    delete data;
}