# Set Calculator

This is a simple command-line Ruby application that implements a Set Calculator using Binary Search Trees (BST). The program allows users to maintain up to 3 sets denoted by X, Y, and Z, each holding exclusively numerical values. No duplicate elements are allowed under any circumstances.

## Features

1. Maintain up to 3 sets (X, Y, Z) printed in ascending order using in-order traversal of the corresponding BST.
2. Implement set operations: union, intersection, deep copying, and element insertion using BSTs.
3. Continuous command loop execution according to the provided command list.

## Usage

Upon starting the program, all 3 sets (X, Y, Z) are initially empty. The program runs a command loop where users can execute the following commands:

- `X values`: Initialize set X with comma-separated list of numeric arguments.
- `Y values`: Initialize set Y with comma-separated list of numeric arguments.
- `Z values`: Initialize set Z with comma-separated list of numeric arguments.
- `a i`: Insert numeric argument i in set X while preserving BST properties.
- `r`: Rotate contents of sets: Y takes content of X, Z takes content of Y, X takes previous content of Z.
- `s`: Switch contents of sets X and Y.
- `u`: Take the union of sets X and Y, storing the result in set X.
- `i`: Take the intersection of sets X and Y, storing the result in set X.
- `c`: Recursively deep copy set X into set Y.
- `l aString`: Apply a one-argument lambda expression to every element of set X and output the result.
- `q`: Quit the program.


## Note

- BSTs are not required to be balanced.
- Ensure code readability and maintainability.
- Avoid unnecessary definition of data structures.

Thank you for using the Set Calculator!
