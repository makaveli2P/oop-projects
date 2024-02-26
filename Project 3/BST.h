//
// Created by dagarw7 on 11/10/2022.
//

#ifndef PROJECT3_BST_H
#define PROJECT3_BST_H
#include <string.h>
#include <vector>
#include <unordered_map>
#include "Node.h"

using namespace std;
class BST{
public:
    int size;
    Node* root;

    BST();                              //default constructor

    BST(aString value);                 //convenience/parameterized constructor

    BST(const BST& curr_tree);          // copy constructor
    Node* deepCopy(Node* ptr);

    ~BST();
    void recursiveDelete(Node* aNode);

    void insert(aString& value);
    void recursiveInsert(Node* &ptr, aString& value);

    void uni(Node* ptr);
    void recursiveUnion(Node* copyFrom);

    void printTree();
    void printInorder(Node* tree_root);

    bool same(Node* aRoot);
    bool sameTree(Node* root1, Node* root2);

    void toVec(Node* aNode, std::vector<aString>& val) const;
    static bool aString_comp(const std::string &lhs, const std::string &rhs);

//    Node* leftmost(Node* aNode);
//    Node* nextInOrder(Node* aNode, aString* val);

};

#endif //PROJECT3_BST_H
