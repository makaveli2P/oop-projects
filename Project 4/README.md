# Assembly Language Interpreter (ALI) for Simple Assembly Language (SAL) in C++

This project implements an Assembly Language Interpreter (ALI) for a Simple Assembly Language (SAL) using C++11. ALI is a virtual machine that interprets SAL programs and executes them. SAL has a set of instructions specified below.

## Instructions

- `DEC symbol`: Declares a symbolic variable consisting of alphabetical characters.
- `LDA symbol`: Loads a 32-bit word from the data memory address of the symbol into the accumulator.
- `LDB symbol`: Loads a 32-bit word from the data memory address of the symbol into register B.
- `LDI value`: Loads the integer value into the accumulator register. The value could be negative.
- `STR symbol`: Stores the content of the accumulator into data memory at the address of the symbol.
- `XCH`: Exchanges the content of registers A and B.
- `JMP number`: Transfers control to the instruction at address number in program memory.
- `JZS number`: Transfers control to the instruction at address number if the zero-result bit is set.
- `JVS number`: Transfers control to the instruction at address number if the overflow bit is set.
- `ADD`: Adds the content of registers A and B. The sum is stored in A. The overflow and zero-result bits are set or cleared as needed.
- `HLT`: Terminates program execution.

## Components

- **Memory**: A 32-bit, word-addressable memory (RAM) for data and source code, holding 256 32-bit words.
- **Accumulator (A)**: A 32-bit register.
- **Additional register (B)**: A 32-bit register.
- **Program Counter (PC)**: An 8-bit program counter (PC).
- **Zero-result bit**: This bit is set if the last ADD instruction produced a zero result.
- **Overflow bit**: This bit is set whenever an ADD instruction produces an overflow.

## Execution

The execution of the ALI consists of the following steps:

1. Prompt the user for a file name in the current directory.
2. Read a SAL program from the file. The program is stored in the memory starting at address 0.
3. Execute a command loop consisting of the following commands:
   - `s`: Execute a single line of code.
   - `a`: Execute all instructions from the current PC value until a halt instruction is encountered.
   - `q`: Quit the command loop.

## Usage

To use this ALI interpreter, compile the source code and run the executable. Follow the on-screen prompts to execute SAL programs.

## Note

Ensure that the SAL program files follow the correct syntax and contain valid instructions.

**Disclaimer**: This project was developed as part of a coursework assignment and may have limitations or bugs.
