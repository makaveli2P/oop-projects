# ALI: an interpreter and visualizer for SAL (C++17)

ALI is an interpreter for SAL (Simple Assembly Language). It shows how each SAL instruction moves data through the hardware: memory, the program counter (PC), the instruction register (IR), the decoder, the registers A and B, the ALU, the zero bit Z and the overflow bit V.

Each step gives a trace of micro-operations, for example "Read Memory[128] (X): 7" and then "Write A: 0 -> 7". The terminal visualizer (TUI) shows the trace one micro-operation at a time on a diagram of the hardware. The classic text mode prints the same trace as text.

## Build

You need a C++17 compiler, CMake 3.25 or newer, and Ninja. CMake downloads FTXUI v7.0.3 for the visualizer and Catch2 v3 for the tests.

```sh
cd "Project 4"
cmake --preset debug          # or: release, asan
cmake --build --preset debug
ctest --preset debug
```

| Preset    | Build type | Notes                                                    |
|-----------|------------|----------------------------------------------------------|
| `debug`   | Debug      | The default for development.                             |
| `release` | Release    | Optimized.                                               |
| `asan`    | Debug      | Adds AddressSanitizer and UndefinedBehaviorSanitizer.    |
| `tsan`    | Debug      | Adds ThreadSanitizer. Use it to examine the animation thread. |

Each preset writes its files to `build/<preset>/`. Git ignores the `build/` folder.

The CMake options are:

- `SAL_BUILD_TUI` (default `ON`): build the terminal visualizer with FTXUI. With `OFF`, ALI has only the classic mode, and CMake does not download FTXUI.
- `SAL_BUILD_TESTS` (default `ON`): build the Catch2 tests.
- `SAL_SANITIZE` (default `OFF`): add `-fsanitize=address,undefined`.
- `SAL_SANITIZE_THREAD` (default `OFF`): add `-fsanitize=thread`. Do not set it together with `SAL_SANITIZE`.

Our own targets compile with `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow`, and warnings are errors. Third-party targets do not get these flags.

## Usage

```sh
./build/debug/app/ali programs/test1.sal          # the terminal visualizer
./build/debug/app/ali                             # choose a program from ./programs
./build/debug/app/ali --plain programs/test1.sal  # the classic text mode
./build/debug/app/ali --plain                     # the classic mode asks for the file name
```

The classic mode accepts three commands:

| Command | Action                                                                                         |
|---------|------------------------------------------------------------------------------------------------|
| `s`     | Execute one instruction. ALI prints the instruction, the micro-operations, A, B, PC, Z, V, the code memory and the data memory. |
| `a`     | Execute instructions until the machine halts. After 1000 steps, ALI asks if it must continue. Then ALI prints the source and the data. |
| `q`     | Quit. The end of the input also quits.                                                          |

If the program has errors, ALI prints each error with its line number and column, and exits with code 1.

Without `--plain`, ALI starts the terminal visualizer. The next section describes it.

The `programs/` folder has these sample programs:

| File            | What it shows                                                        |
|-----------------|----------------------------------------------------------------------|
| `test1.sal`     | Repeated addition. The third ADD overflows, and JVS jumps to HLT.    |
| `test2.sal`     | Integer division 342 / 3 by repeated addition of -3. RESULT = 114.   |
| `test3.sal`     | A Fibonacci loop. At the end, PREV = 267914296 and RESULT = 433494437. |
| `countdown.sal` | A loop that counts down from 5. JZS ends the loop.                   |
| `overflow.sal`  | INT32_MAX + 1. ADD sets V, and JVS sets FLAG to 1.                   |
| `swap.sal`      | XCH, comments, lower-case opcodes, and a program without HLT.        |

## The terminal visualizer

The visualizer shows how each instruction moves data through the hardware. It executes one micro-operation at a time. The terminal must have at least 100 columns and 30 rows. If the terminal is smaller, ALI asks you to enlarge it.

This frame shows the third `ADD` of `test1.sal`. The ALU adds A and B, and the 32-bit result wraps:

```
 ALI · test1.sal                                                                          speed 5 micro-ops/s    Paused
╭─ Program ────────────────╮╭─ Datapath ─────────────────────────────────────────────────╮╭─ Memory ───────────────────╮
│                          ││                                                            ││                            │
│     0  DEC INITIAL       ││                                                            ││ DATA                       │
│     1  DEC ADDER         ││                                                            ││  128 INITIAL    2001000000 │
│     2  LDI 1000000       ││                                                            ││  129 ADDER      1000000000 │
│     3  STR INITIAL       ││                                                            ││  130–255  free             │
│     4  XCH               ││                                                            ││                            │
│     5  LDI 1000000000    ││  ┌ PC ──┐   ┌ Memory ───┐   ┌ IR ───────┐   ┌ Decoder ──┐  ││ CODE                       │
│     6  STR ADDER         ││  │    7 │   │ [7]       │   │ ADD       │   │ ADD       │  ││    0–12  program           │
│ ▶   7  ADD               ││  └───┬──┘   └─────┬─────┘   └─────┬─────┘   └─────┬─────┘  ││   13–127  empty            │
│     8  JVS 12            ││      │            │               │               │        ││                            │
│     9  STR INITIAL       ││      │            │               │               │        ││                            │
│    10  LDB ADDER         ││      └──┬─────────┴────┬──────────┴──┬─────────┬──┴────┐   ││                            │
│    11  JMP 7             ││         │ -1293967296▶ │             │         │       │   ││                            │
│    12  HLT               ││         │              │             ▼         │       │   ││                            │
│    13  end               ││  ┌──────┴────┐   ┌─────┴─────┐   ┌───┴───┐   ┌─┴─┐   ┌─┴─┐ ││                            │
│                          ││  │ 2001000000│   │ 1000000000│   │   +   │   │ 0 │   │ 0 │ ││                            │
│                          ││  └ A ────────┘   └ B ────────┘   └ ALU ──┘   └ Z ┘   └ V ┘ ││                            │
│                          ││                                                            ││                            │
│                          ││                                                            ││                            │
│                          ││  ADD                                    ● ● ● ● ● ○ ○ ○ ○  ││                            │
│                          ││  Add B to A; set Z and V                                   ││                            │
│                          ││  address 7, source line 8                                  ││                            │
│                          ││                                                            ││                            │
│                          ││                                                            ││                            │
│                          ││                                                            ││                            │
│                          ││                                                            ││                            │
│                          ││                                                            ││                            │
│                          ││                                                            ││                            │
│                          ││                                                            ││                            │
╰──────────────────────────╯╰────────────────────────────────────────────────────────────╯╰────────────────────────────╯
  A 2001000000     B 1000000000     PC 7     Z 0     V 0                                                        decimal
  ▸ ALU: 2001000000 + 1000000000 = 3001000000, wraps to -1293967296                         step 18  ·  micro-op 5 of 9

  space next  s step  b back  r run  g to end  +/- speed  R reset  x hex  ⏎ breakpoint  q quit
```

### The screen

- **Header**: the file name, the animation speed and the state: Ready, Paused, Running or Halted. A halted machine shows the reason: "HLT executed" (green) or "ran past end of program" (amber).
- **Program**: the instructions with their addresses. The addresses start at 0. They are the numbers that the jump instructions use. `▶` marks the instruction that executes now (amber) or the next instruction. `●` marks a breakpoint. `■` marks the place where the machine halted. The last row, `end`, is the address after the program.
- **Datapath**: the hardware units PC, Memory, IR, Decoder, A, B, ALU, Z and V on one internal bus. The active micro-operation colors its source units cyan and its target units amber. It also colors the bus between them amber, and writes the value that moves next to the bus. An arrow points to the target. Below the diagram, the instruction card shows the instruction, its description and one dot for each micro-operation.
- **Memory**: the data words (128 and up) with their symbol names, and a summary of the code words (0 to n-1). `▸` and cyan mark a word that the active micro-operation reads. `◂` and amber mark a word that it writes. `•` and cyan mark the symbol that DEC tells about; DEC changes no data, so its value is not amber. A value that changed in this step is amber. If a name and the widest value do not fit on one row, the value goes on a second row, so that the pane shows the full name.
- **Registers**: A, B, PC, Z and V. A value that changed in this step is amber.
- **Narration**: the sentence for the active micro-operation, for example "Read Memory[130] (NTHTERM): 43", and the position "step N · micro-op k of m".
- **Message**: the result of the last command, for example "Stopped at the breakpoint at address 7."
- **Keys**: the key help.

The colors have one meaning each. Gray is for frames and labels. Cyan is for a source. Amber is for a target, the path and changed values. Green is for a normal halt. Red is for breakpoints and runtime faults.

### Keys

| Key                | Action                                                                  |
|--------------------|-------------------------------------------------------------------------|
| `space` or `n`     | Show the next micro-operation. At the end of an instruction, start the next instruction. |
| `s`                | Complete the current instruction, or execute one complete instruction.  |
| `b`                | Go back one instruction. ALI keeps the last 4096 instructions.          |
| `r`                | Start or pause the animated run. The run stops at a breakpoint or a halt. |
| `+` and `-`        | Make the animation faster or slower (1 to 50 micro-operations each second). |
| `g`                | Run to the end without animation. After 1000 instructions, ALI stops and shows a message. Press `g` again to run 1000 more. |
| `R` (shift-r)      | Reset the machine. The breakpoints stay.                                |
| `x`                | Show the words in hexadecimal or in decimal.                           |
| `↑` / `↓` or `k` / `j` | Select a source line.                                               |
| `Enter`            | Set or remove a breakpoint on the selected line.                        |
| Mouse click        | Set or remove a breakpoint on the line that you click.                  |
| `q` or `Esc`       | Quit.                                                                   |

A breakpoint stops a run before its instruction executes. A run that starts on a breakpoint does not stop there at once.

### Errors

If the program has errors, ALI shows an error screen. The screen shows each error with its line number, its column and the source line. If the errors do not fit on the screen, the last row tells how many errors it does not show. Press `q` to quit. Then ALI prints all the errors in the terminal and exits with code 1.

### The --render developer flag

The `--render` flag renders one frame of the complete UI as text and exits. It needs no real terminal, so it is useful for tests and for screenshots:

```sh
ali --render <file> [--steps N] [--ops N] [--keys KEYS] [--size WxH] [--color]
```

| Option        | Meaning                                                                  |
|---------------|--------------------------------------------------------------------------|
| `--steps N`   | First, execute N complete instructions.                                  |
| `--ops N`     | Then, show N more micro-operations.                                      |
| `--keys KEYS` | Then, apply each character as a key press. `E` is the Enter key.         |
| `--size WxH`  | The size of the frame. The default is `120x36`.                          |
| `--color`     | Keep the ANSI color codes. Without it, ALI removes them.                 |

For example, `ali --render programs/test1.sal --steps 17 --ops 5 --size 120x32` gives the frame above.

## The SAL machine

### Hardware

- **Memory**: one array of 256 words of 32 bits. Addresses 0-127 hold the program, one instruction for each address, from address 0. Addresses 128-255 hold data. A code cell holds the decoded instruction, its source text and its source line number. A data cell holds a signed 32-bit integer.
- **A** and **B**: signed 32-bit registers.
- **PC**: an 8-bit program counter.
- **Z** and **V**: the zero bit and the overflow bit. Only ADD changes them.
- The machine also has a halt state with a reason, and a step counter.

### Instructions

| Instruction  | Action                                                                  |
|--------------|-------------------------------------------------------------------------|
| `DEC symbol` | Declare a symbol. At run time, DEC changes no data. It only advances the PC. |
| `LDA symbol` | A <- memory[symbol].                                                    |
| `LDB symbol` | B <- memory[symbol].                                                    |
| `LDI value`  | A <- value. The value is a decimal integer with an optional sign.       |
| `STR symbol` | memory[symbol] <- A.                                                    |
| `XCH`        | Swap A and B.                                                           |
| `JMP number` | PC <- number.                                                           |
| `JZS number` | PC <- number if Z is 1. Otherwise PC <- PC + 1.                         |
| `JVS number` | PC <- number if V is 1. Otherwise PC <- PC + 1.                         |
| `ADD`        | A <- A + B. Set Z and V.                                                |
| `HLT`        | Halt the machine.                                                       |

### The load step

ALI assembles the complete program before it executes the first instruction. The assembler never stops at the first error. It reports all errors, each with a line number and a column. These are the rules:

- Opcodes are case-insensitive: `lda`, `Lda` and `LDA` are the same. Symbols are case-sensitive: `X` and `x` are two symbols.
- A symbol contains letters only.
- A comment starts at `;` or `#` and continues to the end of the line.
- Blank lines and comment lines do not take an address. Only instructions take addresses.
- ALI accepts CRLF line ends and spaces at the end of a line.
- The number of operands must be correct.
- A program has 1-128 instructions.
- DEC gives the data addresses 128, 129, 130, ... in the order of the DEC lines. ALI does this at load time. A duplicate DEC is an error. More than 128 symbols is an error.
- LDA, LDB and STR must name a declared symbol. The DEC line can be before or after the use.
- An LDI value must fit in a signed 32-bit integer.
- A jump target must be the address of an instruction in the program.

### Execution

- **ADD**: ALI calculates the sum in 64 bits. A gets the two's-complement 32-bit result. V is 1 when the exact sum is outside the 32-bit range. Z is 1 when the 32-bit result is 0. For example, 2147483647 + 1 gives -2147483648 with V = 1. The code has no signed-overflow undefined behavior.
- **HLT**: the machine halts. The PC stays at the address of HLT.
- **End of program**: if the PC moves past the last instruction, the machine halts with the reason "ran past end of program".
- **Halted machine**: a step on a halted machine does nothing.

## Architecture

```
Project 4/
  core/     static library sal_core (namespace sal). No terminal I/O.
  tui/      static library sal_tui (namespace sal::tui): the visualizer.
  app/      the executable "ali": the argument parser, the classic mode and the link to sal_tui.
  tests/    Catch2 v3 tests.
  programs/ sample SAL programs.
```

### core

- `assembler.hpp`: `sal::assemble(source)` and `sal::assemble_file(path)` return an `AssembleResult`. It holds a `std::shared_ptr<const Program>` or a list of `ParseError`. The assembler never throws.
- `instruction.hpp`: the abstract class `Instruction` and one `final` subclass for each opcode. `SymbolInstruction` is the base for DEC, LDA, LDB and STR. `JumpInstruction` is the base for JMP, JZS and JVS. Each subclass has a virtual `execute()` and a virtual `describe()`. The program owns the instructions with `std::unique_ptr<const Instruction>`.
- `program.hpp`: `Program` is the immutable result of the assembler. Its constructor checks the invariants that `Machine` relies on: 1 to 128 instructions, no empty instruction, at most 128 symbols with unique names, and symbol i at address 128 + i. If an invariant is false, the constructor throws `std::invalid_argument`. Thus, code that makes a `Program` without the assembler cannot cause undefined behavior in `Machine`.
- `datapath.hpp`: `Datapath` is the only way for an instruction to change the machine. Each method (`read_reg`, `write_reg`, `read_flag`, `write_flag`, `read_mem`, `write_mem`, `alu_add`, `report_symbol`) changes the state and adds one micro-operation to the trace. This is the hook point for the visualizer. If `read_mem` or `write_mem` gets an address that is not a data address, it throws a private `Datapath::Fault`. The fault stops the instruction before it changes more state. `Machine::step()` catches the fault and halts the machine with the reason "runtime fault". The `Halt` micro-operation then holds the address that caused the fault. The assembler cannot make such a program, but the public API can.
- `micro_op.hpp`: `MicroOp` is a `std::variant` of small structs: `Fetch`, `Decode`, `RegRead`, `RegWrite`, `FlagRead`, `FlagWrite`, `MemRead`, `MemWrite`, `AluAdd`, `PcWrite`, `SymbolInfo` and `Halt`. `sal::describe(op)` makes one sentence from a micro-operation.
- `machine.hpp`: `Machine::step()` does fetch, decode, execute and the PC update. It returns a `StepResult` with the address and the micro-operations in order. `Machine::run(limit)` returns the number of steps and the stop reason.
- `alu.hpp`: `sal::add32()` is the pure, `constexpr` 32-bit addition.

A `Machine` is a value type. A copy is an independent snapshot of the registers, the flags and the memory. All copies share one immutable `Program`, so a copy is cheap. The TUI keeps snapshots to step back.

The trace of `LDA X` is: `Fetch`, `Decode`, `MemRead`, `RegWrite`, `PcWrite`. The trace of `ADD` is: `Fetch`, `Decode`, `RegRead` A, `RegRead` B, `AluAdd`, `RegWrite` A, `FlagWrite` Z, `FlagWrite` V, `PcWrite`.

### tui

The visualizer uses only the public API of `sal_core`. It has two layers.

The first layer has no FTXUI code, so the tests can use it without a terminal:

- `controller.hpp`: `Controller` holds the machine, the history and the UI state (the cursor, the breakpoints, the speed, the selection and the number base). To show a part of an instruction, the controller executes the complete instruction, keeps a snapshot of the machine before it, and applies the first k micro-operations to a copy of the snapshot (`apply_micro_op()`). The copy is the state on the screen. The back command restores a snapshot.
- `route.hpp`: `route_of()` gives the source units, the target units and the label of a micro-operation. It uses the other micro-operations of the same instruction. For example, the `RegWrite` of `LDA` gets its value from Memory, the `RegWrite` of `ADD` from the ALU, and the `RegWrite` of `LDI` from the Decoder.
- `keys.hpp`: `handle_key()` maps a key to a controller command. The TUI and `--render` use the same function.
- `ticker.hpp`: `Ticker` is the animation thread.

The second layer uses FTXUI: `theme` (the colors), `diagram` (the datapath on a character grid), `views` (the panes and the screens), `render` (the `--render` mode) and `app` (the event loop and the program list).

**Threads.** The FTXUI loop runs on the main thread, and only the main thread uses the controller. The ticker thread sleeps on a condition variable for one interval. Then it calls `App::PostEvent()` with a private tick event, which FTXUI makes thread-safe. The loop receives the event on the main thread and calls `Controller::tick()`. The ticker and the loop share only the fields of the ticker, and a mutex protects them. When you quit, the loop exits first, then `Ticker::stop()` wakes and joins the thread, and then the objects are destroyed. The `tsan` preset runs the ticker tests under ThreadSanitizer.

### tests

The tests examine each instruction, the ADD edge cases, each load error and its line number, CRLF, comments, blank lines, the halt states, a step on a halted machine, snapshot copies, the order of the micro-operations, and the final state of each sample program.

The visualizer tests need no terminal. They examine the micro-operation cursor, the back command and the history limit, the breakpoints, the animated run, the 1000-step limit of `g`, the key bindings, the route of each micro-operation and the ticker thread. They also render frames to text at several sizes and make sure that each line fits.

## History

This project started as a course project for object-oriented programming in C++11. The original code is at the git tag `college-original`. This version is a new implementation. It keeps the object-oriented design (one class for each instruction) and the SAL specification, and it corrects the defects of the original.
