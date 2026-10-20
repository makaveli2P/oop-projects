# ALI: an emulator and visualizer for SAL (Ruby)

ALI (Assembly Language Interpreter) is an emulator for SAL (Simple Assembly Language). SAL runs on a small machine: one memory of 256 words, the registers A and B, a program counter (PC), a zero bit Z and an overflow bit V.

ALI shows how each SAL instruction moves data through this hardware. Each step gives a trace of micro-operations, for example "Read Memory[128] (X): 7" and then "Write A: 0 -> 7". The terminal visualizer shows the trace one micro-operation at a time on a diagram of the hardware. The classic mode is the command loop of the original assignment, and it prints the same trace as text.

This is the Ruby version. "Project 4" is the same machine in C++17. The two versions use the same SAL semantics, and they give the same results for the sample programs.

## Setup

### 1. Use Ruby 3.4 or newer

On macOS, the system Ruby in `/usr/bin` is 2.6, which is too old. If you use it, `bundle` stops with "Could not find 'bundler' (4.0.20)". Install Ruby with Homebrew and put it first on your `PATH`:

```sh
brew install ruby
echo 'export PATH="/opt/homebrew/opt/ruby/bin:/opt/homebrew/lib/ruby/gems/4.0.0/bin:$PATH"' >> ~/.zshrc
exec zsh
ruby -v        # must show ruby 3.4 or newer
```

Do this one time only.

### 2. Install the development gems

Run these commands from the repository root:

```sh
cd "Project 2"
bundle config set --local path vendor/bundle
bundle install
```

The program itself uses only the Ruby standard library (`io/console` and `optparse`). Bundler installs the development gems: minitest, rake and RuboCop. Git ignores the `vendor/bundle/` and `.bundle/` folders.

```sh
bundle exec rake test     # the tests
bundle exec rake lint     # RuboCop
bundle exec rake          # both
```

## Usage

```sh
bin/ali programs/test1.sal          # the terminal visualizer
bin/ali                             # choose a program from ./programs
bin/ali --plain test1               # the classic mode
bin/ali --plain                     # the classic mode asks for the file name
ruby main.rb                        # main.rb is the same program as bin/ali
```

ALI opens the file name as you type it. If that file does not exist, ALI also tries the name with `.txt` and with `.sal`. Thus `test1`, `test1.txt` and `programs/test1.sal` all work.

### The classic mode

The classic mode asks for the file name ("Enter the SAL file name: "), unless you give the file on the command line. Then it accepts three commands:

| Command | Action |
|---------|--------|
| `s`     | Execute one instruction. ALI prints the instruction, its micro-operations, A, B, PC, Z, V, the code memory and the data memory. |
| `a`     | Execute instructions until the machine halts. After each 1000 steps, ALI asks if it must continue. Then ALI prints the registers, the source code and the data. |
| `q`     | Quit. The end of the input also quits. |

If the file does not exist, ALI asks for the name again. If the program has errors, ALI prints each error with its line number and its column, and exits with status 1.

An example with piped input:

```sh
printf 's\na\ny\nq\n' | bin/ali --plain test2.txt
```

### The terminal visualizer

The visualizer is the default mode. It executes one micro-operation at a time. The terminal must have at least 100 columns and 30 rows. If the terminal is smaller, ALI asks you to enlarge it.

This frame shows the third `ADD` of `test1.sal`. The ALU adds A and B, and the 32-bit result wraps:

```
 ALI · test1.sal                                                              decimal  ·  speed 5 micro-ops/s    Paused
╭─ Program ────────────────╮╭─ Datapath ─────────────────────────────────────────────────╮╭─ Memory ───────────────────╮
│                          ││                                                            ││                            │
│     0  DEC INITIAL       ││                                                            ││ DATA                       │
│     1  DEC ADDER         ││                                                            ││  128 INITIAL    2001000000 │
│     2  LDI 1000000       ││ ┌ PC ──┐  ┌Memory─┐  ┌ IR ───────────┐  ┌ Decoder ──────┐  ││  129 ADDER      1000000000 │
│     3  STR INITIAL       ││ │    7 │  │ [7]   │  │ ADD           │  │ ADD           │  ││  130–255  free             │
│     4  XCH               ││ └───┬──┘  └───┬───┘  └───────┬───────┘  └───────┬───────┘  ││                            │
│     5  LDI 1000000000    ││     │         │              │                  │          ││ CODE                       │
│     6  STR ADDER         ││     └──┬──────┴───────┬──────┴──────┬─────────┬─┴─────┐    ││      0–12  program         │
│ ▶   7  ADD               ││        │       -1293967296 ▶        │         │       │    ││    13–127  empty           │
│     8  JVS 12            ││        │              │             ▼         │       │    ││                            │
│     9  STR INITIAL       ││ ┌──────┴────┐   ┌─────┴─────┐   ┌───┴───┐   ┌─┴─┐   ┌─┴─┐  ││                            │
│    10  LDB ADDER         ││ │ 2001000000│   │ 1000000000│   │   +   │   │ 0 │   │ 0 │  ││                            │
│    11  JMP 7             ││ └ A ────────┘   └ B ────────┘   └ ALU ──┘   └ Z ┘   └ V ┘  ││                            │
│    12  HLT               ││                                                            ││                            │
│    13  end               ││                                                            ││                            │
│                          ││   ADD                                  ● ● ● ● ● ○ ○ ○ ○   ││                            │
│                          ││   Add B to A; set Z and V                                  ││                            │
│                          ││   address 7, source line 8                                 ││                            │
│                          ││                                                            ││                            │
│                          ││                                                            ││                            │
│                          ││                                                            ││                            │
│                          ││                                                            ││                            │
│                          ││                                                            ││                            │
╰──────────────────────────╯╰────────────────────────────────────────────────────────────╯╰────────────────────────────╯
  A 2001000000      B 1000000000      PC 7       Z 0     V 0                               step 18  ·  micro-op 5 of 9
  ▸ ALU: 2001000000 + 1000000000 = 3001000000, wraps to -1293967296

  space next  s step  b back  r run  g to end  +/- speed  R reset  x hex  ⏎ breakpoint  q quit
```

The screen has these parts:

- **Header**: the file name, the base of the numbers (decimal or hexadecimal), the animation speed and the state: Ready, Paused, Running or Halted. A halted machine shows the reason: "HLT executed" (green), "ran past end of program" (amber) or "runtime fault" (red).
- **Program**: the instructions with their addresses. The addresses start at 0. They are the numbers that the jump instructions use. `▶` marks the instruction that executes now (amber) or the next instruction. `●` marks a breakpoint. `■` marks the place where the machine halted. The last row, `end`, is the address after the program. The selected line has a dark background.
- **Datapath**: the units PC, Memory, IR, Decoder, A, B, ALU, Z and V on one internal bus. The active micro-operation colors its source units cyan and its target units amber. It also colors the bus between them amber, and writes the value that moves next to the bus. An arrow points to the target. Below the diagram, the instruction card shows the instruction, its description and one dot for each micro-operation. Before the first step, the card tells you the first key to press. Each line of the card stays inside the pane. A long description continues on a second line.
- **Memory**: the data words (128 and up) with their symbol names, and a summary of the code words. `▸` and cyan mark a word that the active micro-operation reads. `◂` and amber mark a word that it writes. `•` marks the symbol that DEC tells about. A value that changed in this step is amber. If the data words do not fit, the pane shows the words around the active word, and "↑ n more" and "↓ n more".
- **Registers**: A, B, PC, Z and V. A value that changed in this step is amber. At the right, "step N · micro-op k of m" tells where you are.
- **Narration**: the sentence for the active micro-operation. It has the full row, and its values are in the same base as the panes. If the sentence is too long, ALI makes the symbol name shorter, and the values stay complete.
- **Message**: the result of the last command, for example "Stopped at the breakpoint at address 7."
- **Keys**: the key help.

The colors have one meaning each. Gray is for frames and labels. Cyan is for a source. Amber is for a target, the path and changed values. Green is for a normal halt. Red is for breakpoints and faults.

### Keys

| Key | Action |
|-----|--------|
| `space` or `n` | Show the next micro-operation. At the end of an instruction, start the next instruction. |
| `s` | Complete the current instruction, or execute one complete instruction. |
| `b` | Go back one instruction. ALI keeps the last 4096 instructions. |
| `r` | Start or pause the animated run. The run stops at a breakpoint or a halt. |
| `+` and `-` | Make the animation faster or slower (1 to 50 micro-operations each second). |
| `g` | Run to the end without animation. After 1000 instructions, ALI stops and shows a message. Press `g` again to run 1000 more. |
| `R` (shift-r) | Reset the machine. The breakpoints stay. |
| `x` | Show the words in hexadecimal or in decimal. |
| `↑` / `↓` or `k` / `j` | Select a source line. |
| `Enter` | Set or remove a breakpoint on the selected line. |
| Mouse click | Set or remove a breakpoint on the line that you click. |
| `q`, `Esc` or `Ctrl-C` | Quit. |

A breakpoint stops a run before its instruction executes. A run that starts on a breakpoint does not stop there at once.

If the program has errors, ALI shows an error screen with each error, its source line and a `^` under the column. Press `q` to quit. Then ALI prints all the errors in the terminal and exits with status 1.

### The --render developer mode

The `--render` flag draws one frame of the visualizer as text and exits. It needs no terminal, so the tests and this README use it:

```sh
bin/ali --render FILE [--steps N] [--ops N] [--keys KEYS] [--size WxH] [--color]
```

| Option | Meaning |
|--------|---------|
| `--steps N` | First, execute N complete instructions. |
| `--ops N` | Then, show N more micro-operations. |
| `--keys KEYS` | Then, apply each character as a key press. `E` is Enter, `U` is Up and `D` is Down. |
| `--size WxH` | The size of the frame. The default is `120x36`. |
| `--color` | Keep the ANSI color codes. Without it, ALI writes plain text. |

For example, `bin/ali --render programs/test1.sal --steps 17 --ops 5 --size 120x30` gives the frame above. This frame at the minimum size shows `STR RESULT` in `test2.sal`, when the memory writes the word:

```
bin/ali --render programs/test2.sal --steps 16 --ops 4 --size 100x30
```

```
 ALI · test2.sal                                          decimal  ·  speed 5 micro-ops/s    Paused
╭─ Program ─────────────╮╭─ Datapath ────────────────────────────────────╮╭─ Memory ───────────────╮
│                       ││                                               ││                        │
│     2  DEC RESULT     ││                                               ││ DATA                   │
│     3  DEC INCREMENTOR││                                               ││  128 INITIAL       342 │
│     4  LDI 342        ││ ┌ PC ──┐  ┌Memory─┐  ┌ IR ─────┐  ┌ Decoder ┐ ││  129 SUB            -3 │
│     5  STR INITIAL    ││ │   16 │  │ [130] │  │STR RESU…│  │STR RESU…│ ││◂ 130 RESULT          1 │
│     6  LDI -3         ││ └───┬──┘  └───┬───┘  └────┬────┘  └────┬────┘ ││  131 INCREMENTOR     1 │
│     7  STR SUB        ││     │         ▲           │            │      ││  132–255  free         │
│     8  LDI 0          ││     └──┬──────┴─────┬─────┴───┬─────┬──┴──┐   ││                        │
│     9  STR RESULT     ││        │ 1 ▶        │         │     │     │   ││ CODE                   │
│    10  LDI 1          ││        │            │         │     │     │   ││      0–22  program     │
│    11  STR INCREMENTOR││ ┌──────┴────┐ ┌─────┴─────┐ ┌─┴─┐ ┌─┴─┐ ┌─┴─┐ ││    23–127  empty       │
│    12  JZS 22         ││ │         1 │ │         0 │ │ + │ │ 0 │ │ 0 │ ││                        │
│    13  LDA INCREMENTOR││ └ A ────────┘ └ B ────────┘ └ALU┘ └ Z ┘ └ V ┘ ││                        │
│    14  LDB RESULT     ││                                               ││                        │
│    15  ADD            ││                                               ││                        │
│ ▶  16  STR RESULT     ││   STR RESULT                      ● ● ● ● ○   ││                        │
│    17  LDA SUB        ││   Store A into RESULT (address 130)           ││                        │
│    18  LDB INITIAL    ││   address 16, source line 17                  ││                        │
│    19  ADD            ││                                               ││                        │
│    20  STR INITIAL    ││                                               ││                        │
│    21  JMP 12         ││                                               ││                        │
│    22  HLT            ││                                               ││                        │
│    23  end            ││                                               ││                        │
╰───────────────────────╯╰───────────────────────────────────────────────╯╰────────────────────────╯
  A 1               B 0               PC 16      Z 0     V 0           step 17  ·  micro-op 4 of 5
  ▸ Write Memory[130] (RESULT): 0 -> 1

  space next  s step  b back  r run  g to end  +/- speed  R reset  x hex  ⏎ breakpoint  q quit
```

## Sample programs

The original files `test1.txt`, `test2.txt` and `test3.txt` stay in this folder. The `programs/` folder has copies of them with the `.sal` extension, and three more programs from "Project 4".

| File | What it shows | Result |
|------|---------------|--------|
| `test1.txt` | Repeated addition. The third ADD overflows, and JVS jumps to HLT. | A = -1293967296, V = 1, INITIAL = 2001000000, 20 steps |
| `test2.txt` | Integer division 342 / 3 by repeated addition of -3. | RESULT = 114, INITIAL = 0, 1154 steps |
| `test3.txt` | A Fibonacci loop. | PREV = 267914296, RESULT = 433494437, NTHTERM = 0, 522 steps |
| `programs/countdown.sal` | A loop that counts down from 5. JZS ends the loop. | COUNT = 0, LOOPS = 5 |
| `programs/overflow.sal` | INT32_MAX + 1. ADD sets V, and JVS sets FLAG to 1. | SUM = -2147483648, FLAG = 1 |
| `programs/swap.sal` | XCH, comments, lower-case opcodes, and a program without HLT. | FIRST = 20, SECOND = 10 |

## The SAL machine

### Hardware

- **Memory**: one array of 256 words. Addresses 0-127 hold the program, one instruction for each address, from address 0. Addresses 128-255 hold data. A code word holds the decoded instruction, its source text and its source line number. A data word holds a signed 32-bit integer.
- **A** and **B**: signed 32-bit registers.
- **PC**: an 8-bit program counter. It holds the address of the next instruction.
- **Z** and **V**: the zero bit and the overflow bit. Only ADD changes them.
- The machine also has a halt state with a reason, and a step counter.

### Instructions

| Instruction | Action |
|-------------|--------|
| `DEC symbol` | Declare a symbol. At run time, DEC changes no data. It only advances the PC. |
| `LDA symbol` | A <- memory[symbol]. |
| `LDB symbol` | B <- memory[symbol]. |
| `LDI value` | A <- value. The value is a decimal integer with an optional sign. |
| `STR symbol` | memory[symbol] <- A. |
| `XCH` | Swap A and B. |
| `JMP number` | PC <- number. |
| `JZS number` | PC <- number if Z is 1. Otherwise PC <- PC + 1. |
| `JVS number` | PC <- number if V is 1. Otherwise PC <- PC + 1. |
| `ADD` | A <- A + B. Set Z and V. |
| `HLT` | Halt the machine. |

### The load step

ALI assembles the complete program before it executes the first instruction. The assembler does not stop at the first error. It reports all errors, each with a line number and a column. These are the rules:

- Opcodes are case-insensitive: `lda`, `Lda` and `LDA` are the same. Symbols are case-sensitive: `X` and `x` are two symbols.
- A symbol contains letters only.
- A comment starts at `;` or `#` and continues to the end of the line.
- Blank lines and comment lines do not take an address. Only instructions take addresses.
- ALI accepts CRLF line ends, a UTF-8 byte order mark and spaces at the end of a line.
- The number of operands must be correct.
- A program has 1-128 instructions.
- DEC gives the data addresses 128, 129, 130, ... in the order of the DEC lines. ALI does this at load time. A duplicate DEC is an error. More than 128 symbols is an error.
- LDA, LDB and STR must name a declared symbol. The DEC line can be before or after the use.
- An LDI value must fit in a signed 32-bit integer.
- A jump target must be the address of an instruction in the program.

### Execution

- **ADD**: ALI calculates the exact sum. A gets the two's-complement 32-bit result. V is 1 when the exact sum is outside the 32-bit range. Z is 1 when the 32-bit result is 0. For example, 2147483647 + 1 gives -2147483648 with V = 1, and -2147483648 + -2147483648 gives 0 with Z = 1 and V = 1.
- **HLT**: the machine halts. The PC stays at the address of HLT.
- **End of program**: if the PC moves past the last instruction, the machine halts with the reason "ran past end of program".
- **Halted machine**: a step on a halted machine does nothing.
- **Run all**: the `a` command and the `g` key stop after 1000 steps and ask before they continue.

## Architecture

```
Project 2/
  main.rb            the entry point of the course project; the same as bin/ali
  bin/ali            the executable
  lib/ali.rb         the core: no terminal I/O
  lib/ali/           the core classes, the classic mode and the command line
  lib/ali/tui/       the terminal visualizer
  test/              minitest tests
  programs/          sample SAL programs
  test1.txt ...      the original sample programs
```

### The core

The core has no I/O. Only `Assembler.assemble_file` reads a file.

- `word.rb`: `Word` keeps values in 32 bits (`Word.wrap`, `Word.fits?`, `Word.format`). Ruby integers have no size limit, so this module does the work that the hardware does. `ALU.add` returns an `AddResult` (a `Data` object) with the result, the exact sum, and the Z and V bits.
- `memory_map.rb`: the layout of the 256 words.
- `instructions.rb`: the abstract class `Instruction` and one subclass for each opcode. `SymbolInstruction` is the base of DEC, LDA, LDB and STR. `JumpInstruction` is the base of JMP, JZS and JVS. `PlainInstruction` is the base of XCH, ADD and HLT. Each subclass declares its opcode with the class macro `opcode "LDA", operand: :symbol`, and has `execute(datapath)` and `describe`. `INSTRUCTION_SET` maps each opcode to its class. The instruction objects are frozen.
- `assembler.rb`: `Assembler.assemble(source)` returns an `AssembleResult` that holds a `Program` or a list of `ParseError`. The assembler never raises an error for bad input.
- `program.rb`: `Program` is the immutable result of the assembler. Its constructor checks the invariants that `Machine` relies on: 1 to 128 instructions, at most 128 unique symbols, and symbol i at address 128 + i.
- `datapath.rb`: `Datapath` is the only way for an instruction to change the machine. Each method (`read_reg`, `write_reg`, `read_flag`, `write_flag`, `read_mem`, `write_mem`, `alu_add`, `report_symbol`) changes the working state and adds one micro-operation to the trace. An access to an address that is not a data address raises `Datapath::Fault`. `Machine#step` catches it and halts the machine with the reason "runtime fault". The assembler cannot make such a program, but the public API can.
- `micro_op.rb`: one `Data` class for each micro-operation: `Fetch`, `Decode`, `RegRead`, `RegWrite`, `FlagRead`, `FlagWrite`, `MemRead`, `MemWrite`, `AluAdd`, `PcWrite`, `SymbolInfo` and `Halt`. Each one has `to_s` (one sentence) and `apply(state)` (the state after the micro-operation).
- `machine.rb`: `State` is an immutable `Data` snapshot of the registers, the flags, the memory, the halt reason and the step counter. `Machine#step` does fetch, decode, execute and the PC update, and returns a `StepResult` with the address and the micro-operations. `Machine#run(limit:)` returns a `RunResult`.

The trace of `LDA X` is `Fetch`, `Decode`, `MemRead`, `RegWrite`, `PcWrite`. The trace of `ADD` is `Fetch`, `Decode`, `RegRead` A, `RegRead` B, `AluAdd`, `RegWrite` A, `FlagWrite` Z, `FlagWrite` V, `PcWrite`.

### The front ends

- `plain_mode.rb`: `PlainMode` is the classic s/a/q command loop. `SourceFile.resolve` finds the file from the name that the user types.
- `cli.rb`: `CLI` parses the arguments with `OptionParser` and starts one of the three modes.

The visualizer in `lib/ali/tui/` has two layers. The first layer has no terminal code, so the tests use it directly:

- `controller.rb`: `Controller` holds the machine, the history and the UI state (the cursor, the breakpoints, the speed, the selection and the number base). To show a part of an instruction, the controller executes the complete instruction, keeps the `State` before it, and applies the first k micro-operations to that state with `MicroOp#apply`. The result is the state on the screen. The back command restores a `State` from the history.
- `route.rb`: `Routing.route_of` gives the source units, the target units and the label of a micro-operation. It uses pattern matching on the `Data` classes, and the other micro-operations of the same instruction. For example, the `RegWrite` of LDA gets its value from Memory, the `RegWrite` of ADD from the ALU, and the `RegWrite` of LDI from the Decoder.
- `keys.rb`: `Keys.handle` maps a key to a controller command. The app and `--render` use the same table.
- `canvas.rb`: `Canvas` is a grid of cells with a style for each cell. It makes plain text or text with ANSI colors. `Theme` holds the five colors.
- `diagram.rb`, `screen.rb` and `other_screens.rb` draw the datapath, the main screen, the error screen, the program list and the "terminal too small" screen on a canvas.
- `render.rb`: the `--render` mode.

The second layer touches the terminal:

- `terminal.rb`: `Terminal` puts the terminal in raw mode, uses the alternate screen and the mouse, decodes the keys, and writes only the rows that changed since the last frame.
- `app.rb`: `App` is the event loop.

**One thread.** The app has no ticker thread. `Terminal#read_keys` waits for input with `IO.select`. During an animated run, the timeout of the wait is the time to the next tick, so the wait is also the animation clock. A resize signal (SIGWINCH) writes to a pipe, and the pipe wakes the wait. When you quit, the terminal goes back to its normal mode, also after an error.

## Design decisions

The assignment does not tell these things. ALI uses the same decisions as "Project 4".

- **LDA and LDB load a word.** The original README says "loads byte". The machine has only 32-bit words, so ALI loads the full word.
- **DEC at load time.** The assembler gives each symbol its address before the program runs. Thus a jump back over a DEC does not move the symbol, and a symbol can be used before its DEC line.
- **Addresses start at 0.** Jump targets, the PC and the program pane use the same numbers. The original printed PC + 1.
- **Blank lines and comments take no address.** Without this rule, a blank line would be an instruction.
- **Two halt reasons.** HLT and "ran past end of program" are different states. HLT keeps the PC at its address. The PC is never a fake value.
- **A step limit.** A program that loops forever does not hang ALI. The `a` command asks to continue after each 1000 steps.
- **A hand-written renderer.** I examined the terminal UI gems in October 2026. The `tty-reader` and `tty-cursor` gems have had no release since 2020. The `ratatui_ruby` gem (1.5.2) is maintained, but it is a native extension for a Rust library under the LGPL. The `curses` gem (1.7.0) is maintained, but it needs a native build and the curses library. The screen of ALI is a fixed layout of text, so a small renderer with `io/console` is enough. It has no native dependency, and `--render` gives exactly the frame that the terminal shows.

## Bugs of the original version

The original code is at the git tag `college-original`. A review of that code found these bugs. This version corrects all of them, and the tests examine each correction.

| ID | Bug in the original | Correction |
|----|---------------------|------------|
| P2-01 | A blank line, a comment, a lower-case opcode or an unknown opcode made the `a` command loop forever, because the dispatcher had no `else` branch and the PC did not change. | The assembler checks the complete program at load time. Opcodes are case-insensitive. Blank lines and comments take no address. An unknown opcode is an error with its line number. |
| P2-02 | The overflow test of ADD was off by one (`sum > 2**31`), and the wrap value was wrong (`sum - 2**31`). In `test1.txt`, A got 853516352. The zero bit used the sum before the wrap. | `ALU.add` wraps to 32 bits with two's complement. V is 1 when the exact sum is outside the range. Z uses the 32-bit result. `test1.txt` now gives A = -1293967296. |
| P2-03 | An undeclared symbol or a missing operand crashed LDA, LDB and STR with a `TypeError`. | The assembler reports "undeclared symbol 'X'" and the number of operands, with the line and the column. |
| P2-04 | Jump targets had no check. `JMP -1` crashed, a target past the end stopped silently, and `JMP abc` jumped to 0. | A jump target must be the address of an instruction. The assembler reports other targets. |
| P2-05 | DEC allocated an address each time it executed. A jump back over a DEC moved the symbol and lost its value. A 129th DEC wrote past the 256 words. | DEC allocates at load time. A duplicate DEC and more than 128 symbols are errors. At run time, DEC changes no data. |
| P2-06 | The loader did not enforce the 128-word program area. A long program overwrote the data area or made the memory array longer. | A program has at most 128 instructions. The memory is always 256 words. |
| P2-07 | LDI accepted any text (`to_i` gave 0 for `abc`) and any size. | An LDI value must be a decimal integer that fits in 32 bits. |
| P2-08 | The `a` command had no limit, so a loop that never ends hung the emulator. | `a` stops after each 1000 steps and asks to continue. The visualizer `g` key stops after 1000 instructions. |
| P2-09 | The loader always added `.txt` to the name, so `test1.txt` opened `test1.txt.txt` and crashed. A missing file crashed. The end of the input crashed `gets.chomp`. | ALI opens the name as typed, then tries `.txt` and `.sal`. If no file exists, ALI asks again. The end of the input quits. |
| P2-10 | The step output showed PC + 1, which did not agree with the jump targets. The JZS output had no separator ("Program Counter at 13Zero bit"). | One function prints the state for all instructions. The PC is the real address. |
| P2-11 | `s` said "HLT encountered" when the program ended without HLT. `a` printed the state only after HLT. HLT set the PC past the program. | The machine has a halt reason: "HLT executed", "ran past end of program" or "runtime fault". `a` always prints the final state. HLT keeps the PC. |
| P2-12 | There were no expected results and no tests. | The tests have golden results for the three programs, and tests for each error case. The results agree with "Project 4". |
| P2-13 | Execution and printing were mixed. The dispatcher was written two times, and the state print was copied 11 times. This blocked a visualizer. | A core without I/O, a `Datapath` that records each micro-operation, and two front ends that use the same `Machine#step`. |

The review also noted that the original README told you to run `ruby ali_emulator.rb` and to type `program.sal`. The file is `main.rb`, and the code added `.txt`. This README gives the correct commands.

## Tests

`bundle exec rake test` runs 111 tests with warnings on (`ruby -w`). The tests examine:

- the 32-bit arithmetic and each ADD edge case;
- each load rule and each error message, with its line and column;
- the trace of each instruction, the halt states, a step on a halted machine, a runtime fault, the step limit, and the immutable snapshots;
- the final state of each sample program;
- the classic mode with scripted input, and the command line;
- the controller (micro-operation cursor, back, history limit, breakpoints, animated run, run to end, speed), the routes, the keys and the key decoder;
- frames of each sample program at four sizes, and a check that each line has the width of the frame.

`bundle exec rake lint` runs RuboCop with the minitest and rake plugins. It reports no offenses.

## History

This project started as a course project for object-oriented programming in Ruby. The original code is at the git tag `college-original`. This version is a new implementation. It keeps the object-oriented design (one class for each instruction, with a shared superclass), the command loop and the SAL specification, and it corrects the defects of the original.
