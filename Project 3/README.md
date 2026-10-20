# Sets: a string set calculator and BST visualizer (C++17)

Sets is a calculator for two sets of strings, S1 and S2. Each set is an unbalanced binary search tree (BST) of `String` objects. `String` is a custom string class that owns a `char*` buffer.

The terminal visualizer (TUI) shows each command step by step on drawings of the two trees: each string comparison on the search path, each new node, each node that a command frees, each node that a copy makes, and the swap of the two roots. The classic mode is the command loop of the course project.

## Build

You need a C++17 compiler, CMake 3.25 or newer, and Ninja. CMake downloads FTXUI v7.0.3 for the visualizer and Catch2 v3.16.1 for the tests.

```sh
cd "Project 3"
cmake --preset debug          # or: release, asan, tsan
cmake --build --preset debug
ctest --preset debug
```

| Preset    | Build type | Notes                                                         |
|-----------|------------|---------------------------------------------------------------|
| `debug`   | Debug      | The default for development.                                  |
| `release` | Release    | Optimized.                                                    |
| `asan`    | Debug      | Adds AddressSanitizer and UndefinedBehaviorSanitizer.         |
| `tsan`    | Debug      | Adds ThreadSanitizer. Use it to examine the animation thread. |

Each preset writes its files to `build/<preset>/`. Git ignores the `build/` folder.

The CMake options are:

- `STRSET_BUILD_TUI` (default `ON`): build the terminal visualizer with FTXUI. With `OFF`, Sets has only the classic mode, and CMake does not download FTXUI.
- `STRSET_BUILD_TESTS` (default `ON`): build the Catch2 tests.
- `STRSET_SANITIZE` (default `OFF`): add `-fsanitize=address,undefined`.
- `STRSET_SANITIZE_THREAD` (default `OFF`): add `-fsanitize=thread`. Do not set it together with `STRSET_SANITIZE`.

Our own targets compile with `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual`, and warnings are errors. Third-party targets do not get these flags.

## Usage

```sh
./build/debug/app/strset           # the terminal visualizer
./build/debug/app/strset --plain   # the classic command loop
```

### The classic mode

The classic mode reads one command on each line. After each command, it prints a sentence about the result, and then S1 and S2 in alphabetical order.

| Command    | Action                                                                                   |
|------------|------------------------------------------------------------------------------------------|
| `e`        | Erase S1. Sets frees each node of S1.                                                    |
| `s`        | Switch S1 and S2. Only the two root pointers change.                                     |
| `b`        | Tell if S2 is a proper subset of S1.                                                     |
| `c`        | Copy S1 into S2. Sets frees the old nodes of S2 and makes a deep copy of S1.             |
| `l`        | List the strings of S1 and S2 in alphabetical order.                                     |
| `a <text>` | Add a string to S1. Without a string on the line, Sets asks for it on the next line.     |
| `u`        | Union: S1 = S1 ∪ S2.                                                                     |
| `i`        | Intersection: S1 = S1 ∩ S2.                                                              |
| `q`        | Print S1 and S2 and quit. The end of the input also quits.                               |
| `?`        | Show the commands.                                                                       |

An example:

```
$ printf 'a pear\na apple\nc\na fig\nb\nq\n' | ./build/debug/app/strset --plain
...
Command: Added "fig" to S1.
S1 = {"apple", "fig", "pear"}
S2 = {"apple", "pear"}

Command: Yes: S2 is a proper subset of S1.
S1 = {"apple", "fig", "pear"}
S2 = {"apple", "pear"}

Command: S1 = {"apple", "fig", "pear"}
S2 = {"apple", "pear"}
Quitting the set calculator.
```

## The terminal visualizer

Without `--plain`, Sets starts the visualizer. The terminal must have at least 80 columns and 20 rows. If the terminal is smaller, Sets asks you to enlarge it.

Press a command key. The command runs, and then the screen shows its steps one at a time. This frame shows `a peach`. The search starts at the root, and the third comparison finds the first different character, at index 3:

```
 Sets · a peach   Add a string to S1                                    speed 1.4 steps/s    Paused
╭─ S1 · 5 strings ───────────────────────────────╮╭─ S2 · 0 strings ───────────────────────────────╮
│                                                ││                                                │
│                         mango                  ││                                                │
│      ┌────────────────────┴────────────┐       ││                                                │
│    apple                              pear     ││                                                │
│      └──────┐                                  ││                                                │
│           cherry                               ││                                                │
│             └──────┐                           ││                     empty                      │
│                   kiwi                         ││                                                │
│                                                ││                                                │
│                                                ││                                                │
│                                                ││                                                │
│                                                ││                                                │
│                                                ││                                                │
│                                                ││                                                │
╰────────────────────────────────────────────────╯╰────────────────────────────────────────────────╯
  ▸ "peach" < "pear": go left. 'c' < 'r' at index 3.                                   step 3 of 5
    Compare: peach  <  pear

  › a add   e erase   s switch   b subset   c copy   l list   u union   i intersect
  space step   ← back   r run   g end   +/- speed   ? help   q quit
```

In a real terminal, `pear` is cyan (the node that the step examines), the path from `mango` to `pear` is amber, and in the row "Compare", the `c` of `peach` is amber and the `r` of `pear` is cyan.

This frame shows `c` (copy). Each node has its serial number, so you can see that each node of S2 is a new allocation, not a pointer to a node of S1:

```
 Sets · c   Copy S1 into S2                                             speed 1.4 steps/s    Paused
╭─ S1 · 7 strings · root #1 ─────────────────────╮╭─ S2 · 6 strings · root #8 ─────────────────────╮
│                                                ││                                                │
│                      mango #1                  ││                      mango #8                  │
│                    ┌────┴────┐                 ││                    ┌────┘                      │
│                 apple #2  pear #3              ││                 apple #9                       │
│                    └─┐                         ││                    └─┐                         │
│                  cherry #4                     ││                  cherry #10                    │
│                 ┌────┴────┐                    ││                 ┌────┴─────┐                   │
│             banana #7  kiwi #5                 ││             banana #11  kiwi #12               │
│                         ┌─┘                    ││                          ┌─┘                   │
│                       fig #6                   ││                       fig #13                  │
│                                                ││                                                │
│                                                ││                                                │
│                                                ││                                                │
│                                                ││                                                │
╰────────────────────────────────────────────────╯╰────────────────────────────────────────────────╯
  ▸ New node #13 in S2 copies node #6 "fig". It has its own String buffer.             step 7 of 9
    Copied 6 of 7 nodes

  › a add   e erase   s switch   b subset   c copy   l list   u union   i intersect
  space step   ← back   r run   g end   +/- speed   ? help   q quit
```

### The screen

- **Header**: the command on the screen and its description, the animation speed, and the state: Ready, Running, Paused or Done.
- **S1 and S2**: the two trees. The title shows the number of strings. For `c` and `s`, the labels and the titles also show the node serial numbers.
- **Narration**: the sentence for the current step and the position "step N of M".
- **Progress**: the result so far. For `a`, the two strings of a comparison, or the search path. For `b` and `i`, each lookup with ✓ or ✗. For `u`, the added strings. For `l`, the strings in order. For `e` and `c`, the number of freed and copied nodes.
- **Command line**: the command keys. After `a`, it is the prompt "Add to S1 ›".
- **Keys**: the playback keys.

The colors have one meaning each. Gray is for frames, connectors and labels. Cyan is for the node that a step examines. Amber is for a new node, the search path and the changed links. Green is for a lookup that found its string and for a "yes" verdict. Red is for a node that a command frees or removes and for a "no" verdict.

What each command shows:

| Command | Steps                                                                                         |
|---------|-----------------------------------------------------------------------------------------------|
| `a`     | Each comparison on the path, with the first different character, then the new link or "already in S1". |
| `e`     | Each node of S1, leaves first, just before Sets frees it.                                     |
| `c`     | Each old node of S2 that Sets frees, then each new node of S2, in preorder, with its serial number and the node of S1 that it copies. |
| `u`     | Each string of S2 (in preorder), the search for it in S1, and the new link.                   |
| `i`     | Each string of S1 (in order), the search for it in S2, and each removal. If the removed node has two children, its in-order successor takes its place. |
| `b`     | Each string of S2, the search for it in S1, and the verdict.                                  |
| `s`     | The two roots before and after the swap. No node moves.                                       |
| `l`     | An in-order walk of S1 and then of S2.                                                        |

### Trees of all sizes

Sets uses the first style that shows the complete tree:

1. **Columns**: the root at the top. Each node has its own columns, in alphabetical order from left to right.
2. **Compact**: also top-down, but each subtree is packed against its sibling (a "tidy tree"). A wider tree fits.
3. **Sideways**: one row for each node, in alphabetical order from the top. A left child is above its parent and a right child is below it. The indentation is the depth. A long chain (for example, after sorted inserts) uses this style. If the tree has more rows than the pane, the pane follows the current node and shows "⋮ N above" and "⋮ N below". On the last step, the pane stays on the last node that the command used, so the result stays visible.

During one command, the style and the columns of the nodes do not change, so the nodes do not jump while a tree grows or shrinks. The `c` command has two phases: it frees the old S2, and then it builds the new S2. Each phase has its own fixed layout. A label longer than 12 columns (28 in the Sideways style) is cut with "…". The narration shows a long string as an excerpt around the first different character.

A command with more than 2000 steps is not animated. The screen then shows the result and a message.

### Keys

| Key                | Action                                                         |
|--------------------|----------------------------------------------------------------|
| `a`                | Open the prompt. Type the string, then press `Enter`. `Esc` cancels. |
| `e` `s` `b` `c` `l` `u` `i` | Run the command and start its animation.              |
| `space`, `→` or `Enter` | Show the next step. This pauses the run.                  |
| `←`                | Show the previous step.                                        |
| `r`                | Start or pause the animated run. At the end, `r` plays the command again. |
| `g` or `End`       | Go to the last step.                                           |
| `+` and `-`        | Make the animation faster or slower (0.7 to 20 steps each second). |
| `?`                | Show or close the help.                                        |
| `q` or `Esc`       | Quit.                                                          |

### The --render developer flag

The `--render` flag renders one frame of the complete UI as text and exits. It needs no real terminal, so it is useful for tests and for screenshots:

```sh
strset --render [--setup CMDS] [--keys KEYS] [--ticks N] [--size WxH] [--color]
```

| Option         | Meaning                                                                         |
|----------------|---------------------------------------------------------------------------------|
| `--setup CMDS` | First, run the commands without animation. Separate them with `;`, for example `"a pear;a apple;c"`. |
| `--keys KEYS`  | Then, apply each character as a key press. `{enter}`, `{esc}`, `{bs}`, `{left}`, `{right}`, `{end}` and `{space}` are named keys. |
| `--ticks N`    | Then, apply N animation ticks.                                                  |
| `--size WxH`   | The size of the frame. The default is `120x36`.                                 |
| `--color`      | Keep the ANSI color codes. Without it, Sets removes them.                       |

For example, these commands give the two frames above:

```sh
strset --render --setup "a mango;a apple;a pear;a cherry;a kiwi" --keys "apeach{enter}{space}{space}" --size 100x22
strset --render --setup "a mango;a apple;a pear;a cherry;a kiwi;a fig;a banana" --keys "c{space}{space}{space}{space}{space}{space}" --size 100x22
```

## Architecture

```
Project 3/
  core/     static library strset_core (namespace strset). No terminal I/O.
  tui/      static library strset_tui (namespace strset::tui): the visualizer.
  app/      the executable "strset": main.cpp, the argument parser and the link to strset_tui.
  tests/    Catch2 v3 tests.
```

### core

- `aString.h`: `String`, the string class of the course. Its only data members are `int size_` and `char* chars_`. It owns its buffer with `new[]` and `delete[]`, and implements the complete rule of five: copy constructor, copy assignment (copy and swap), move constructor, move assignment, and destructor. It also has `compare()` (bytes as `unsigned char`, as `strcmp()` does), the six comparison operators and `first_difference()`. It does not use `std::string`. An empty `String` allocates nothing, and a moved-from `String` is empty.
- `Node.h`: `Node` holds one `String` and the two child links. A node cannot be copied or moved: `BST` copies a tree node by node. Each node gets a serial number, and `Node::live_count()` counts the nodes that exist. The tests use the count to find leaks.
- `BST.h`: `BST` is a set of `String` objects. It has a deep-copy copy constructor, copy assignment, move operations and a destructor. It has `insert`, `contains`, `remove`, `clear`, `assign_copy`, `unite`, `intersect`, `is_proper_subset_of`, `swap` and in-order and preorder walks. Each operation can tell a `BstObserver` about each step. Without an observer (`nullptr`), the operations do no extra work.
- `trace.h`: `Recorder` is a `BstObserver` that writes a `Trace`: a list of steps. Each step has an `Event` (a `std::variant` of small structs such as `Compare`, `Link`, `Free`, `Copy`, `Remove` and `Swap`) and a `Snapshot` of both trees. Steps that do not change a tree share one snapshot. `describe(event)` makes the narration sentence.
- `calculator.h`: `SetCalculator` holds S1 and S2 and runs the commands. Each command returns an `Outcome` (the sentence, the verdict of `b`, and a "changed" flag) and can record a `Trace`.
- `command.h`: `parse_command()` parses one command line.
- `classic.h`: `run_classic(in, out)` is the classic command loop. It uses streams, so the tests run it on strings.

### tui

The visualizer uses only the public API of `strset_core`. It has two layers.

The first layer has no FTXUI code, so the tests can use it without a terminal:

- `controller.h`: `Controller` runs a command on the `SetCalculator` at once and records its trace. The screen then shows the snapshot of the current step. Thus, to step back is only to show an earlier step. The controller also makes the node colors (`marks()`) and the progress row.
- `tree_layout.h`: `draw_tree()` draws a tree on a grid of character cells in the Columns, Compact or Sideways style.
- `keys.h`: `handle_key()` maps a key to a controller command. The TUI and `--render` use the same function.
- `ticker.h`: `Ticker` is the animation thread.

The second layer uses FTXUI: `theme` (the colors), `views` (the panes and the screens), `render` (the `--render` mode) and `app` (the event loop).

**Threads.** The FTXUI loop runs on the main thread, and only the main thread uses the controller. The ticker thread sleeps on a condition variable for one interval. Then it calls `App::PostEvent()` with a private tick event, which FTXUI makes thread-safe. The loop receives the event on the main thread and calls `Controller::tick()`. When you quit, the loop exits first, then `Ticker::stop()` wakes and joins the thread, and then the objects are destroyed. The `tsan` preset runs the ticker tests under ThreadSanitizer.

### tests

The core tests examine the rule of five of `String` and `BST` (deep copies, independent copies, self-assignment, moves), the comparison of `String` (prefixes, `'\0'` bytes, bytes above 127), insert, remove (a leaf, one child, two children), the set operations, each command of the calculator and of the classic loop, the command parser, the trace of each command, the step limit, the live node count (no leaks), and the copy and the destructor of a 12000-node chain on a 256 KiB thread stack.

The visualizer tests need no terminal. They examine the cursor, the run and the ticks, the speed limits, each command key, the prompt, the node colors, the three tree styles (with 200 random trees for the Compact style), stable columns, long strings and long chains, and the ticker thread. They also render frames to text at several sizes and make sure that each line fits.

## Design decisions

The course text does not say these things, so Sets does this:

- **Ownership of the nodes.** `BST` owns its nodes with `std::unique_ptr<Node>`. The type then tells who owns each node, and the moves, the swap and the exception paths free each node exactly one time without extra code. `String` stays a manual `char*` owner, because the course asks for it. The two classes thus show both ways: manual ownership with the rule of five, and ownership in the type.
- **No recursion.** A chain of `unique_ptr` links frees itself recursively, so a long chain (for example, after 12000 sorted inserts) can overflow the stack. `BST::clear()` frees the nodes without recursion, and so does each walk and the copy. Thus a `Node` always has no children when it is destroyed.
- **The copy.** The copy constructor, copy assignment and `c` copy node by node in preorder. Each node and each `String` buffer is a new allocation, and the copy has the same shape as the original. Copy assignment uses copy and swap: if an allocation throws, the target does not change.
- **Comparison.** Strings compare byte by byte as `unsigned char`, as `strcmp()` does. Upper case comes before lower case ("Zebra" < "apple"). A prefix comes before the longer string ("pea" < "pear"). A `String` can hold `'\0'` bytes.
- **Proper subset.** `b` prints "Yes" if each string of S2 is in S1 and S1 has more strings. Thus an empty S2 is a proper subset of a non-empty S1, and two equal sets give "No". For "No", Sets tells the reason: the first string of S2 that is not in S1, or "S1 and S2 have the same strings".
- **Union and intersection.** `u` inserts the strings of S2 into S1 in preorder. Thus a union into an empty S1 gives a tree with the shape of S2, but with new nodes. `i` removes from S1 each string that is not in S2. A removal relinks the in-order successor; it copies no string.
- **The command line.** `a pear` adds "pear". The rest of the line is the string, so a string can contain spaces. `a` alone asks for the string on the next line. Sets does not add an empty string. The other commands take no argument: `e x` gives a message. An unknown command gives a message. The end of the input works like `q`. CRLF line ends work.
- **Serial numbers, not addresses.** The visualizer shows a serial number for each node, for example `#13`, and not its address. Addresses are long and change on each run; the serial numbers show the same thing: a copy makes new nodes.

## Fixed bugs of the original version

We compiled the original code with `-fsanitize=address,undefined` and ran it. These bugs are verified:

1. **Stack buffer overflow in `a`.** `char val[] {}` is an array with no elements, and `cin >> val` writes the string past it. A 40-character string corrupts the stack, and the program crashes in `printTree()`.
2. **Heap buffer overflow in `c` and `b`.** `deepCopy()` makes `new aString` (a 1-byte buffer) and then `strcpy()`s the full string into it. AddressSanitizer stops `a pear`, `c` at `BST.cpp:29`.
3. **Shared nodes after `u`.** If S1 is empty, `recursiveUnion()` sets `root = copyFrom`, so S1 and S2 share the same nodes. If S1 is not empty, each new node of S1 points to the `aString` of S2. After `a pear`, `s`, `u`, `e`, the command `l` reads freed memory (heap-use-after-free).
4. **Wrong subset result.** `sameTree()` compares the `aString` pointers, not the text, so two non-empty sets are never "the same". `same()` has no `return` statement, which is undefined behavior. With S1 = {apple, pear} and S2 = {pear}, the program prints "S2 is not a proper subset of S1". Also, the method (union, then compare) cannot tell a proper subset from an equal set.
5. **`i` does nothing, and it crashes on an empty set.** The intersection code is commented out. `toVec()` reads `aNode->left` of a null node (SEGV for an empty S1), and it adds the root value for each node.
6. **`e` calls the destructor directly.** `S1->~BST()` ends the life of S1, and the program then uses S1 again (undefined behavior). `size` keeps its old value.
7. **The destructor of `aString` writes to `cout`.** `e` on {apple, pear} prints "applepear" in the middle of the output.
8. **No rule of three.** `aString` has no copy assignment, so an assignment copies the pointer and two objects delete the same buffer. `aString(char*)` with `nullptr` uses `size_` before it has a value. `Node::~Node()` calls `data->~aString()` but does not delete `data`.
9. **Memory leaks.** `s`, `b` and `c` make temporary `BST` objects and do not delete them. `c` drops the old nodes of S2. `Node()` makes an `aString` that `recursiveInsert()` and `deepCopy()` then replace. A duplicate `a` drops its `aString`. S1 and S2 are never deleted. The macOS `leaks` tool finds 8 leaks for `a pear`, `a apple`, `a pear`, `s`, `c`, `q`. The new version has 0 leaks for the same input and the other commands.
10. **An endless loop at the end of the input.** Without `q`, `cin >> command` fails, `command` stays `' '`, and the loop prints the menu without end.
11. **`size` is wrong.** `insert()` never changes `size`, and `s` swaps the roots but not the sizes.
12. **Files that do not build.** `main.h` holds unrelated code that does not compile (it includes `main.cpp` and uses an undeclared `file`). The repository also had a `cmake-build-debug/` folder with Windows build output. This version removes both.

## History

This project started as a course project for object-oriented programming in C++. The original code is at the git tag `college-original`. This version is a new implementation. It keeps the course requirements (the `String` class with `size` and `chars`, the `BST` class with a deep-copy copy constructor and a destructor, the files `aString`, `BST`, `Node` and `main`, and the commands `e`, `s`, `b`, `c`, `l`, `a`, `u`, `i` and `q`), and it corrects the defects of the original.
