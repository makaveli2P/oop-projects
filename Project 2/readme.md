# ALI Emulator

This project is an ALI (Assembly Language Interpreter) emulator implemented in Ruby. It simulates a computer with specific hardware components and allows users to execute SAL (Simple Assembly Language) programs.

## Features

- Prompt the user for a file name in the current directory.
- Read a SAL program from the file and store it in memory starting at address 0.
- Execute a command loop with the following commands:
  - `s`: Execute a single line of code, starting from memory address 0; update the PC, registers, and memory according to the instruction; and print the value of the registers, the zero bit, the overflow bit, and only the memory locations that store source code or program data after the line is executed.
  - `a`: Execute all instructions until a halt instruction is encountered or there are no more instructions to be executed. Print the program’s source code and data used by the program.
  - `q`: Quit the command loop.

## Hardware Components

The ALI emulator includes the following hardware components:

1. **Memory**: 256 32-bit words, word-addressable. Locations 0-127 hold program source code, and locations 128-255 hold program data.
2. **Accumulator (Register A)**: 32-bit register.
3. **Additional register (Register B)**: 32-bit register.
4. **Program Counter (PC)**: 8-bit program counter, holds the address of the next instruction to be executed.
5. **Zero-result bit**: Set if the last ADD instruction produced a zero result.
6. **Overflow bit**: Set whenever an ADD instruction produces an overflow.

## Instruction Set

The SAL instruction set includes the following instructions:

- `DEC symbol`: Declares a symbolic variable.
- `LDA symbol`: Loads byte at data memory address of symbol into the accumulator.
- `LDB symbol`: Loads byte at data memory address of symbol into B.
- `LDI value`: Loads the integer value into the accumulator register.
- `STR symbol`: Stores content of accumulator into data memory at address of symbol.
- `XCH`: Exchanges the content registers A and B.
- `JMP number`: Transfers control to instruction at address number in program memory.
- `JZS number`: Transfers control to instruction at address number if the zero-result bit is set.
- `JVS number`: Transfers control to instruction at address number if the overflow bit is set.
- `ADD`: Adds the content of registers A and B.
- `HLT`: Terminates program execution.

## Usage

To use the ALI emulator, follow these steps:

1. Clone the repository.
2. Run the Ruby script.
3. Enter the name of the SAL program file when prompted.
4. Enter commands (`s`, `a`, or `q`) to execute the program.

## Example

```ruby
# Clone the repository
git clone https://github.com/yourusername/ali-emulator.git

# Run the Ruby script
ruby ali_emulator.rb

# Enter the name of the SAL program file when prompted
Enter the name of the SAL program file: program.sal

# Enter commands to execute the program
s
a
q
