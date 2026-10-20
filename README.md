# Object Oriented Projects

This repository contains four object-oriented projects implemented in Ruby and C++. Each project focuses on different aspects of programming, including data structures, algorithms, and emulation of assembly language instructions. Below is a brief overview of each project:

## Project 1: Set Calculator (Ruby)

This project is a command-line Ruby application that implements a Set Calculator using Binary Search Trees (BST). Users can maintain up to 3 sets denoted by X, Y, and Z, each holding exclusively numerical values. Duplicate elements are not allowed under any circumstances.

## Project 2: ALI Emulator (Ruby)

The ALI Emulator is an Assembly Language Interpreter implemented in Ruby. It simulates a computer with specific hardware components and enables users to execute SAL (Simple Assembly Language) programs. The emulator supports various commands such as executing a single line of code, executing all instructions until a halt instruction is encountered, and quitting the command loop.

## Project 3: String Set Calculator (C++)

This C++ program implements a simple string set calculator using binary search trees (BSTs) to manage sets of strings. The calculator exposes a command-line interface and maintains two sets of strings, S1 and S2. It supports various set operations.

## Project 4: Assembly Language Interpreter (ALI) and Visualizer for SAL (C++17)

This project is an interpreter for SAL (Simple Assembly Language) in modern C++17. A core library assembles a SAL program, executes it step by step, and records each step as a trace of hardware micro-operations (memory, PC, IR, decoder, A, B, ALU, Z, V). A terminal visualizer (FTXUI) shows the trace one micro-operation at a time on a diagram of the hardware, and a classic text mode prints it. The project uses CMake presets, Catch2 tests and AddressSanitizer and ThreadSanitizer builds. The original college version is at the git tag `college-original`.

For detailed instructions on running each project and understanding their functionalities, please refer to the respective README files within each project directory.
