#include<iostream>
#include<cstdlib>

#include "main.cpp"

using namespace std;

int main()
{

    ifstream infile("tvDB.txt");
    if(file.is_open())
    {
        string myArray[];

        for(int i = 0; i < 5; ++i)
        {
            file >> myArray[i];
        }
    }




    int TreeKeys[16] = {50, 76, 21, 4, 32, 64, 15, 52, 14, 100, 83, 2, 3, 70, \
87, 80};

    BST myTree;    // Create the binary search tree

    cout << "Printing the tree in ordder\nBefore adding numbers\n";

    myTree.PrintInOrder();   // print tree in order

    for(int i = 0; i < 16; i++)
    {
        myTree.AddLeaf(TreeKeys[i]);   // copy values into tree
    }

    cout << "Printing the tree in order\nAfter adding numbers\n";

    myTree.PrintInOrder();        // print tree in order

    for(int i = 0; i < 16; i++)
    {
        myTree.AddLeaf(TreeKeys[i]);   // copy values into tree
    }

    cout << "Printing the tree in order\nAfter adding numbers\n";

    myTree.PrintInOrder();        // print tree in order

    cout << endl;

    return 0;
}