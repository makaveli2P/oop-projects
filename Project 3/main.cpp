#include <iostream>
#include <string.h>
#include <limits>

using namespace std;

class Node{
public:
    const char* data;
    Node* left;
    Node* right;

    Node() {
        data = nullptr;
        left = nullptr;
        right = nullptr;
    }
};
class BST{

public:
    Node* root;
    BST(){
        root = nullptr;
    }
    bool isTreeEmpty() {
        if (root == NULL) {
            return true;
        } else {
            return false;
        }
    }

    Node* addElement(Node* temproot, Node* new_new){
        if (temproot == nullptr){
            temproot = new_new;
            return temproot;
        }
        int compare = strcmp(new_new->data, temproot->data);
        if(compare < 0){
                temproot->left = addElement(temproot->left,new_new);
            }
            else if (compare > 0){
                temproot->right = addElement(temproot->right,new_new);
            }
        else {
            return temproot;
        }
        return temproot;
    }

    void inOrder(Node* curr){
        if (curr == nullptr)
            return;
        inOrder(curr->left);
        cout << curr->data <<" ";
        inOrder(curr->right);

    }
};

void printCommands(){
    cout << "==============================================="<< endl;
    cout << "e  -   Erase set"<< endl;
    cout << "s  -   Switch sets" << endl;
    cout << "b  -   Subset operation" << endl;
    cout << "c  -   Copy set" << endl;
    cout << "l  -   List set contents" << endl;
    cout << "a  -   Add element" << endl;
    cout << "u  -   Union" << endl;
    cout << "i  -   Intersection"<< endl;
    cout << "q  -   Quit the set manager" << endl;
    cout << "==============================================="<< endl;
}

char getCommand(){
    char command = ' ';
    cout << endl << "Input: ";
    cin >> command;
    return command;
}

int main() {


    char command = ' ';

    printCommands();
    command = getCommand();

    while (command != 'q'){
        Node* newNode = new Node();
        if(command == 'e'){
            cout << "erasing set" << endl;
        }

        else if(command == 's'){
            cout << "switching sets" << endl;
        }

        else if(command == 'b'){
            cout << "subset" << endl;
        }

        else if(command == 'c'){
            cout << "copy set" << endl;
        }

        else if (command == 'l'){
            cout << "list set contents" << endl;
        }

        else if(command == 'a'){
            cout << "Add element" << endl;
        }

        else if(command == 'u'){
            cout << "union" << endl;
        }

        else if(command == 'i'){
            cout << "intersection" << endl;
        }

        cout << endl << endl;
        cin.ignore(numeric_limits<streamsize>::max(),'\n');
        printCommands();
        command = getCommand();

    }

    cout << "Quitting the Set Manager!" << endl;

    return 0;
}