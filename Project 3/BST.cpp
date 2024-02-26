//
// Created by dagarw7 on 11/10/2022.
//
#include <iostream>
#include <string.h>
#include "BST.h"

BST::BST(){                             // default constructor
    size = 0;
    root = nullptr;
}

BST::BST(aString value){                // parameterized constructor
    root = new Node(value);
    size = 1;
}

BST::BST(const BST& src_tree){              // copy constructor
    root = deepCopy(src_tree.root);
    size = src_tree.size;
}

Node* BST::deepCopy(Node* aNode){           // deep copy function, helper for copy constructor
    if(aNode == nullptr){
        return nullptr;
    }
    else {
        aString* temp = new aString;
        strcpy(temp->chars_,aNode->data->chars_);
        temp->size_ = aNode->data->size_;
        Node* newNode = new Node();
        newNode->data = temp;
        newNode->left = deepCopy(aNode->left);
        newNode->right = deepCopy(aNode->right);
        return newNode;
    }
}


BST::~BST(){                                //destructor for BST
    recursiveDelete(root);
    root = nullptr;
}

void BST::recursiveDelete(Node* aNode){     // recursively delete nodes in a post-order traversal fashion
    if (aNode == nullptr)
        return;
    else{
        recursiveDelete(aNode->left);
        recursiveDelete(aNode->right);
        delete aNode;
    }
}

void BST::insert(aString& value){
    recursiveInsert(root,value);
}

void BST::recursiveInsert(Node* &ptr, aString& value){          // helper function for insert to facilitate a command
    if (ptr == nullptr){
        Node* temp = new Node;
        temp->data = &value;
        temp->right = temp->left = nullptr;
        ptr = temp;
    }
    else{
        int compare = strcmp(value.chars_,ptr->data->chars_);
        if (compare < 0){
            recursiveInsert(ptr->left,value);
        }
        else if (compare > 0) {
            recursiveInsert(ptr->right,value);
        }
    }
}


void BST::printTree() {
    printInorder(root);
}
void BST::printInorder(Node* tree_root){            // in-order/lexicographic printing of BSTs
    if (tree_root)
    {
        printInorder(tree_root->left);
        std::cout << tree_root->data->chars_ << ' ';
        printInorder(tree_root->right);
    }
}

void BST::uni(Node* ptr){
    recursiveUnion(ptr);
}

void BST::recursiveUnion(Node* copyFrom){           // recursive union function inserts values into source, also uses insert
    if (copyFrom == nullptr) return;
    else if(root == nullptr){
        root = copyFrom;
    }
    else{
        insert(*copyFrom->data);
        recursiveUnion(copyFrom->left);
        recursiveUnion(copyFrom->right);
    }
}


bool BST::same(Node* aRoot){
    sameTree(root,aRoot);
}

bool BST::sameTree(Node* root1,Node* root2) {           // function to see id two BSTs are identical or not
    if (root1 == nullptr && root2 == nullptr) {
        return true;
    }
    if (root1 != nullptr && root2 != nullptr) {
        if ((root1->data == root2->data) && sameTree(root1->left, root2->left) &&
            sameTree(root1->right, root2->right)) {
            return true;
        }
    }
    return false;
}

void BST::toVec(Node* aNode, vector<aString>& val) const{
    if (aNode->left!= nullptr){
        toVec(aNode->left,val);
    }
    val.emplace_back(root->data->chars_);
    if (aNode->right != nullptr)
        toVec(aNode->right,val);
}

bool aString_comp(const aString &lhs, const aString &rhs)
{
    return (lhs.size_ == rhs.size_) ? lhs.chars_ < rhs.chars_ : lhs.size_ < rhs.size_;
}

//Node* BST::leftmost(Node* aNode){
//    while(aNode->left != nullptr){
//        aNode = aNode->left;
//    }
//    return aNode;
//}
//Node* BST::nextInOrder(Node* aNode, aString* val){
//    while(aNode && aNode->data <= val){
//        aNode = aNode->right;
//    }
//    nextInOrder(aNode->left, val);
//    return aNode;
//}
