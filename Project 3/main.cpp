// ----------------------------------------------------------------------------------------
// Description: I have created a custom string class 'aString' with appropriate
// data members, constructors and destructors. Same for the 'Node' Class. My
// BST class has utility functions along with the appropriate constructors and destructors.
// ----------------------------------------------------------------------------------------
// Program Flow: There are two objects S1 and S2 with are BST pointers and the interactive
// user will execute the commands on them. A BST root pointer gets created which is a pointer
// to aString. So, BST Node will have 3 attributes, a data field which is aString pointer and
// two Node pointers left and right.
// ----------------------------------------------------------------------------------------
// Assumptions: The interactive user enters valid data.
// ----------------------------------------------------------------------------------------


#include <iostream>
#include <limits>
#include <algorithm>
#include "aString.h"
#include "BST.h"


using namespace std;

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

    BST* S1 = new BST;          // Set S1 which is a BST pointer
    BST* S2 = new BST;          // Set S2 which is a BST pointer

    char command = ' ';

    printCommands();
    command = getCommand();

    while (command != 'q'){
        if(command == 'e'){                         // command e will erase set S1
            S1->~BST();                             // explicitly calling the destructor on the Set S1
        }

        else if(command == 's'){                    // command s will switch the root pointers of BSTs S1 and S2
//            swap(S1->root,S2->root);              // swapping two variables using the std::swap built-in command
            BST* tempTree2 = new BST;
            tempTree2->root = S1->root;             // swapping using the temp assignment
            S1->root = S2->root;
            S2->root = tempTree2->root;
        }


// my idea for subset is to create a tempTree3 BST tree and make it a deep copy of S1. Then take the union of
// tempTree3 and S2. If there were any new values in S2 it will be inserted to tempTree3. Now I check if the two BSTs
// tempTree3 and S1 are identical. If yes, then S2 is a proper subset of S1.
        else if(command == 'b'){
//            cout << "subset" << endl;
            BST* tempTree3 = new BST(*S1);
            tempTree3->uni(S2->root);
            if(S1->same(tempTree3->root)){
                cout << "S2 is a proper subset of S1";
            }
            else{
                cout << "S2 is not a proper subset of S1";
            }
        }

        else if(command == 'c'){                    // command c will deep copy set S1 into set S2
            BST* tempTree = new BST(*S1);           // invoking the copy constructor for S2 and passing S1 as an argument
            S2->root = tempTree->root;
            S2->printTree();
        }

        else if (command == 'l'){                   // l command simple outputs the set contents
            cout << "S1" << endl;
            S1->printTree();
            cout << endl << endl;
            cout << "S2" << endl;
            S2->printTree();
        }

        else if(command == 'a'){
            char val[] {};
            cin >> val;                                 // type 'a someword' to insert
            aString* someval = new aString(val);
            S1->insert(*someval);                   // recursive insertion of an object of aString type into S1
        }

        else if(command == 'u'){
            cout << "union" << endl;
            S1->uni(S2->root);                  // recursively inserts S2 objects into S1 and drops the duplicate items
        }


// my idea for 'i' command is to convert the BST S1 and S2 to a vector and then taking the intersection of those two vectors
// by using set_intersection() function (for this #include <algorithm> is necessary). Then I will store the common values in a
// third vector. Then I will pass those values to S1 using my insert command.
        else if(command == 'i'){
            cout << "intersection" << endl;
            vector<aString> vec1;
            S1->toVec(S1->root,vec1);

            vector<aString> vec2;
            S2->toVec(S2->root,vec2);
//            vector<aString> vec3(vec1.size() + vec2.size());
//            vector<aString>::iterator it, end;

//            std::sort(vec1.begin(), vec1.end(), BST::aString_comp);
//            sort(vec2.begin(), vec2.end(), BST::aString_comp);
//            end = set_intersection(
//                    vec1.begin(), vec1.end(),
//                    vec2.begin(), vec2.end(),
//                    vec3.begin());
//
//            S1->insert(end);


        }
        cout << endl << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;

        cout << "S1" << endl;
        S1->printTree();
        cout << endl << endl;
        cout << "S2" << endl;
        S2->printTree();

        cout << endl << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;

        cin.ignore(numeric_limits<streamsize>::max(),'\n');
        printCommands();
        command = getCommand();

    }
    cout << endl << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;

    cout << "S1" << endl;
    S1->printTree();
    cout << endl << endl;
    cout << "S2" << endl;
    S2->printTree();

    cout << endl << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
    cout << "Quitting the Set Manager!" << endl;


    return 0;
}
