# Object Oriented Projects

This repository contains four object-oriented projects in Ruby and C++. They started as college assignments. Each project now has tests, a classic text mode that follows the assignment, and a terminal visualizer that shows how the data structure or the machine works. The original college code is at the git tag `college-original`.

## Project 1: Set Calculator and BST Visualizer (Ruby)

This project is a set calculator in Ruby 4.0. It keeps three sets of numbers, X, Y and Z. Each set is a hand-written binary search tree (BST). A terminal visualizer draws the three trees and animates each command step by step: the comparison path of an insert, the traversals of union and intersection, the node-by-node deep copy, and the rotate and switch of the sets. The `l` command applies a one-argument lambda to X in an isolated, time-limited process. A classic text mode keeps the command loop of the assignment. The project uses Bundler, Minitest and RuboCop.

## Project 2: ALI Emulator and Visualizer for SAL (Ruby)

This project is an interpreter for SAL (Simple Assembly Language) in Ruby 4.0. It uses the same machine and the same rules as Project 4. A core assembles the program, runs it step by step, and records each step as a trace of hardware micro-operations. A terminal visualizer shows the trace on a diagram of the hardware (memory, PC, IR, decoder, A, B, ALU, Z, V), and a classic text mode keeps the `s`, `a` and `q` commands. The project uses Bundler, Minitest and RuboCop.

## Project 3: String Set Calculator and BST Visualizer (C++17)

This project is a string set calculator in C++17. It keeps two sets of strings, S1 and S2, as binary search trees. As the assignment asks, a custom `aString` class owns its `char*` buffer, and the BST makes deep copies. Both classes follow the rule of five. A terminal visualizer (FTXUI) animates each command: the string comparison at each node, the deep copy into new nodes, union, intersection, the proper-subset check and the erase of nodes. A classic text mode keeps the command loop of the assignment. The project uses CMake presets, Catch2 tests and AddressSanitizer and ThreadSanitizer builds.

## Project 4: Assembly Language Interpreter (ALI) and Visualizer for SAL (C++17)

This project is an interpreter for SAL (Simple Assembly Language) in modern C++17. A core library assembles a SAL program, executes it step by step, and records each step as a trace of hardware micro-operations (memory, PC, IR, decoder, A, B, ALU, Z, V). A terminal visualizer (FTXUI) shows the trace one micro-operation at a time on a diagram of the hardware, and a classic text mode prints it. The project uses CMake presets, Catch2 tests and AddressSanitizer and ThreadSanitizer builds. The original college version is at the git tag `college-original`.

For detailed instructions on running each project and understanding their functionalities, please refer to the respective README files within each project directory.
