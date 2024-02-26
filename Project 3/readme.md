# String Set Calculator

This C++ program implements a simple string set calculator using binary search trees (BSTs) to manage sets of strings. The calculator exposes a command-line interface and maintains two sets of strings, S1 and S2. It supports various set operations as described below.

## Implementation Details

### String Class
- Define a custom `String` class for strings contained in the BSTs.
- The `String` class has two data members: `size` and `chars` of type `int` and `char*` respectively.
- Implement a copy constructor and a destructor to avoid memory leaks.
- Other member functions should be defined as needed.

### BST Class
- Sets are implemented by a class named `BST`.
- Implement the copy constructor and the destructor for `BST`.
- The copy constructor must perform a deep copy, and the destructor must delete all dynamically-allocated objects used exclusively by the receiver.
- Other methods could be added as needed.

### Command Line Interface
- The command-line interface operates in a loop, prompting the user for a command, executing it, and then printing the content of S1 and S2.
- Ensure no memory leaks or dangling pointers occur during the execution of commands.

## Supported Commands

- **e**: Erase set. Deletes the current S1 set.
- **s**: Switch sets. Swaps the sets associated with S1 and S2.
- **b**: Subset operation. Compares S1 and S2 and prints whether S2 is a proper subset of S1.
- **c**: Copy set. Deep copies S1 into S2, deleting the previous content of S2.
- **l**: List set contents. Prints the string values stored in the two sets in alphabetical order.
- **a**: Add element. Adds a new string to S1, preserving the BST properties.
- **u**: Union. Takes the set union of S1 and S2, storing the result in S1.
- **i**: Intersection. Takes the set intersection of S1 and S2, storing the result in S1.
- **q**: Prints sets S1 and S2 and quits the set manager.

## Usage

1. Compile the C++ program.
2. Run the compiled executable.
3. Enter commands as listed above to perform set operations.

## Note

This implementation does not require balancing instances of the `BST` class.
