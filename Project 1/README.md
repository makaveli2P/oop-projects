# Set calculator: three binary search trees (Ruby)

The set calculator keeps three sets of numbers, X, Y and Z. Each set is a binary search tree (BST) that the program implements itself. The calculator has the commands of the assignment: assign, insert, rotate, switch, union, intersection, deep copy and lambda.

The terminal visualizer draws the three trees. It animates each command one step at a time, for example "7 > 5, go right" during an insert. The classic mode is the command loop of the assignment. It prints the three sets after each command.

## Setup

You need Ruby 3.4 or newer. The program uses only the standard library (`io/console`, `prism` and `optparse`). Bundler installs the development gems (minitest, rake and RuboCop) into `vendor/bundle`.

```sh
cd "Project 1"
bundle config set --local path vendor/bundle
bundle install
bundle exec rake          # the tests and RuboCop
bundle exec rake test     # only the tests
bundle exec rake lint     # only RuboCop
```

On macOS, the system Ruby (2.6) is too old. Use the Homebrew Ruby, for example `/opt/homebrew/opt/ruby/bin/ruby` and `/opt/homebrew/opt/ruby/bin/bundle`.

## Usage

```sh
ruby main.rb                    # the terminal visualizer
ruby main.rb --plain            # the classic command loop
printf 'X 5,3,8\nu\nq\n' | ruby main.rb   # input from a pipe: the classic mode
```

If the input or the output is not a terminal, `main.rb` starts the classic mode.

At the start, the three sets are empty. Each command is one line:

| Command      | Action                                                                                         |
|--------------|------------------------------------------------------------------------------------------------|
| `X values`   | Make X a new set with the values. The values are numbers with commas or spaces between them: `X 5,3,8`. `X` alone makes X empty. |
| `Y values`   | The same for Y.                                                                                |
| `Z values`   | The same for Z.                                                                                |
| `a i`        | Insert the number i into X.                                                                    |
| `r`          | Rotate: Y gets the content of X, Z gets the content of Y, and X gets the old content of Z.     |
| `s`          | Switch X and Y.                                                                                |
| `u`          | X gets the union of X and Y.                                                                   |
| `i`          | X gets the intersection of X and Y.                                                            |
| `c`          | Y gets a recursive deep copy of X.                                                             |
| `l aString`  | Call the one-argument lambda `aString` for each element of X, in ascending order. Print the results. X does not change. Example: `l ->(x) { x * 2 }`. |
| `q`          | Quit. The end of the input also quits.                                                         |

A number is an integer (`7`, `-3`) or a decimal (`2.5`, `1e3`). The set names X, Y and Z and the command letters are not case-sensitive. A set has no duplicates. 2 and 2.0 are the same element, and the set keeps the first one.

The classic mode prints the output of the command (the results of `l`, or a note about a duplicate), then the three sets:

```
$ printf 'X 5,3,8,1,4\nY 4,9\nu\nl ->(x) { x * x }\nq\n' | ruby main.rb --plain
X = {1, 3, 4, 5, 8}
Y = {}
Z = {}
X = {1, 3, 4, 5, 8}
Y = {4, 9}
Z = {}
X = {1, 3, 4, 5, 8, 9}
Y = {4, 9}
Z = {}
[1, 9, 16, 25, 64, 81]
X = {1, 3, 4, 5, 8, 9}
Y = {4, 9}
Z = {}
```

If a command is not correct, the classic mode prints one line that starts with `error:`, and the sets do not change.

## The terminal visualizer

The visualizer shows each tree in a panel. The panel shows the tree, the set in ascending order (the in-order traversal) and the number of elements and levels. Below the panels, the narration line tells what the step does. The command line at the bottom is the primary action: type a command and press Enter. The visualizer executes the command and plays its animation.

This frame shows the third step of `a 6`. 6 is less than 8, so the insert goes to the left subtree of 8. The round brackets mark the node that the step compares, here 8:

```
 Set calculator  ·  three binary search trees                             speed 4 steps/s    Paused
╭─ X ────────────────────────────╮╭─ Y ───────────────────────────╮╭─ Z ───────────────────────────╮
│                                ││                               ││                               │
│              [5]               ││           [4]                 ││                               │
│       ┌───────┴───────┐        ││        ┌───┴───┐              ││                               │
│      [3]             (8)       ││       [1]     [9]             ││                               │
│   ┌───┴───┐       ┌───┴───┐    ││                └────┐         ││                               │
│  [1]     [4]     [7]     [9]   ││                   [12]        ││                               │
│                                ││                               ││               ∅               │
│                                ││                               ││                               │
│                                ││                               ││                               │
│                                ││                               ││                               │
│                                ││                               ││                               │
│                                ││                               ││                               │
│ {1, 3, 4, 5, 7, 8, 9}          ││ {1, 4, 9, 12}                 ││                               │
│ 7 elements · height 3          ││ 4 elements · height 3         ││ empty set                     │
╰────────────────────────────────╯╰───────────────────────────────╯╰───────────────────────────────╯

  ▸ 6 < 8, go left.                                                              a 6 · step 3 of 6



  › type a command:  X 5,3,8,1,4  ·  a 7  ·  u  ·  i  ·  c  ·  r  ·  s  ·  l ->(x) { x * 2 }  ·  q
  ⏎ run   space/→ next   ← back   tab play/pause   +/- speed   end skip   ↑/↓ scroll   esc quit
```

### The screen

- **Header**: the name, the animation speed and the state: Ready, Playing, Paused or Done.
- **Panels**: one panel for each set. During `i`, a fourth panel "X ∩ Y" shows the new tree. When the command ends, the new tree becomes X. The title shows where a set moves during `r` and `s`: "▸ to Y" before the move and "◂ from X" after it.
- **Narration**: the sentence for this step and "step k of n".
- **Message**: the results of `l`, a note about a duplicate, or an error.
- **Command line**: the text that you type. When it is empty, it shows examples of the commands.
- **Keys**: the key help.

The colors have one meaning each:

| Color | Meaning                                                                 |
|-------|-------------------------------------------------------------------------|
| Gray  | Frames, labels, branches, and nodes that the command already processed. |
| Cyan  | The node that the step compares or visits.                              |
| Amber | The path from the root to that node, and a set that moves.              |
| Green | A node that the step adds, and the results of `l`.                     |
| Red   | An error.                                                               |

### What each command shows

- **X values** and **a i**: each comparison from the root ("7 > 5, go right"), and then the new node ("Put 7 as the right child of 5."). A duplicate stops at the equal node ("5 = 5: 5 is already in X. Skip it.").
- **u**: a visit of each node of Y in pre-order, and the insert of its value into X, with all its comparisons.
- **i**: a visit of each node of X in pre-order, and a search for its value in Y. If Y has the value, the value goes into the new tree X ∩ Y. At the end, X ∩ Y becomes X.
- **c**: the copy of each node into Y, in the order of the recursion: a node, then its left subtree, then its right subtree.
- **r** and **s**: two steps. The first step shows where each set goes. The second step shows the sets after the move.
- **l**: one step for each element of X, with the result: "f(4) = 8".

### Large trees

The layout gives each node its own columns in in-order sequence. Thus, a left subtree is always to the left of its parent and a right subtree always to the right. If the tree is too wide for its panel, the panel removes the square brackets from the labels. If the tree is still too large, the view scrolls to the node that the step compares. The markers "▴ 3 levels above" and "▾ 9 levels below" show the hidden levels. A row that continues out of the view ends with ‹ or ›. The view never shows a part of a number. Use ↑ and ↓ to scroll more. When you scroll, the view moves sideways to follow the visible rows, so it always shows a part of the tree. The scroll stops at the first and the last row, so the opposite key moves the view at once.

For example, `X 1,2,...,20` makes a chain of 20 levels. The view follows the insert down the chain.

If the set listing does not fit on two lines, it ends with "… 7 more}".

### Keys

When the command line is empty, the space bar and the keys `+` and `-` control the animation. When the command line has text, they go into the text. No command starts with these characters.

| Key                   | Action                                                                  |
|-----------------------|-------------------------------------------------------------------------|
| Enter                 | Execute the command on the command line.                                |
| `space` or `→`        | Show the next step. This pauses the animation.                          |
| `←`                   | Go back one step.                                                       |
| `tab`                 | Play or pause the animation. At the end, `tab` plays the command again. |
| `+` and `-`           | Make the animation faster or slower (1 to 20 steps each second).        |
| `end` and `home`      | Go to the last step or the first step.                                  |
| `↑` and `↓`           | Scroll the trees.                                                       |
| `backspace`           | Delete the last character.                                              |
| `esc`                 | Clear the command line. If it is empty, quit.                           |
| `q` and Enter         | Quit (the command of the assignment). `ctrl-c` also quits.            |

The terminal must have at least 80 columns and 22 rows. If it is smaller, the visualizer asks you to make it larger.

### The --render developer flag

The `--render` flag renders one frame of the screen as text and exits. It needs no terminal, so it is useful for tests and for screenshots:

```sh
ruby main.rb --render [--cmd COMMAND]... [--step N] [--keys KEYS] [--size WxH] [--color]
```

| Option          | Meaning                                                                       |
|-----------------|-------------------------------------------------------------------------------|
| `--cmd COMMAND` | Execute the command. You can give this option more than one time.             |
| `--step N`      | Show step N (from 1) of the last command. Without it, show the last step.     |
| `--keys KEYS`   | Then, apply each character as a key. Use `<enter>`, `<tab>`, `<left>`, `<right>`, `<up>`, `<down>`, `<home>`, `<end>`, `<esc>`, `<bs>` and `<space>` for the other keys. |
| `--size WxH`    | The size of the frame. The default is `120x36`.                              |
| `--color`       | Keep the ANSI color codes.                                                    |

For example, `ruby main.rb --render --cmd "X 5,3,8,1,4,7,9" --cmd "Y 4,1,9,12" --cmd "a 6" --step 3 --size 100x24` gives the frame above.

## The lambda command and its risk

`l aString` must evaluate Ruby code that the user types. Ruby code can do anything that the user can do, for example delete files. Thus, the calculator does not give the text directly to `eval`. It uses four layers of protection:

1. **One lambda only.** Prism (the Ruby parser in the standard library) parses the text. The text must be exactly one lambda with exactly one named parameter: `->(x) { ... }`, `-> x { ... }` or `lambda { |x| ... }`.
2. **An allowlist for the body.** Each node of the body must be on a list: numbers, strings, symbols, arrays, `true`, `false`, `nil`, the parameter, `if`/`unless`/`? :`, `&&`/`||`, and method calls with a receiver. The method must be an operator, one of a small set of Numeric, String and Array methods (`abs`, `round`, `even?`, `to_s`, `size`, `max` and others), or a `Math` function. The body cannot call a method without a receiver (`system`, `eval`, `exit`, `puts`), use `send`, assign a variable, use a block, a loop, a backtick, a global variable, an instance variable, or a constant other than `Math`.
3. **An empty binding.** The calculator builds a canonical copy of the lambda from the checked parts. It evaluates the copy in the binding of an empty `BasicObject`, so `self` has no `Kernel` methods.
4. **A child process with a time limit.** The calculator calls the lambda in a child process (`fork`). If the calls take more than 2 seconds, the calculator stops the child. Thus, an expensive expression such as `x ** 30_000_000` cannot freeze the program, and the lambda cannot change the state of the calculator.

The calculator catches each error. An error in one call (for example `ZeroDivisionError` for `->(x) { 10 / x }` and x = 0) shows as `<ZeroDivisionError>` in the result list, with the message on its own line. A text that is not allowed gives an error message, and nothing executes.

This protection is for a local tool that you use yourself. It is not a sandbox for code from other people. A permitted method can still use much memory for 2 seconds (for example `x.to_s.center(10**9)`). Do not give `l` text that you do not trust.

## Architecture

```
Project 1/
  main.rb                    the entry point: SetCalc::CLI.run(ARGV)
  lib/set_calc.rb            loads the core
  lib/set_calc/
    node.rb                  Node: a value and two links
    bst.rb                   BST: insert, search, in-order and pre-order traversal, height, recursive deep copy
    command.rb               CommandParser: one line of text -> Command, or CommandError
    calculator.rb            Calculator: the three sets and the commands; Result
    safe_lambda.rb           SafeLambda: the checked lambda of the "l" command
    trace.rb                 Snapshot, Panel, Frame, Tracer and NullTracer
    classic.rb               Classic: the command loop of the assignment
    cli.rb                   the options: --plain, --render, --help
    tui/
      player.rb              Player: the frames, the step position, play/pause, speed, the command line
      keys.rb                Keys: terminal bytes -> keys; the key script of --render
      tree_view.rb           TreeView: the layout and the drawing of one tree, with scrolling
      screen.rb              Screen: all the parts of the screen
      canvas.rb              Canvas: a grid of characters with styles; text or ANSI output
      text.rb                Text: clean, cut and wrap text
      render.rb              Render: the --render mode
      app.rb                 App: raw mode, the alternate screen and the event loop
  test/                      minitest tests
```

**The core** (`node.rb` to `calculator.rb`) has no terminal code. `BST` is a hand-written, unbalanced binary search tree. Its walks are iterative, so a chain of 3000 nodes is safe. Only `deep_copy` is recursive, because the assignment asks for a recursive copy. The methods that compare values yield events to an optional block: `[:compare, node_value, :left | :right | :equal]` and `[:insert, value, parent_value, side]`. `BST` includes `Enumerable`, so `each` (in-order) gives `to_a`, `map` and `count`.

**The calculator** executes each command completely. During the command, it tells a tracer about each step: the narration, the marks of the nodes (focus, path, new, done) and the tags of the panels. In the classic mode, the tracer is a `NullTracer` that records nothing.

**The tracer** makes a `Frame` for each step. A frame holds an immutable `Snapshot` of each tree. A tree that did not change since the last frame shares its snapshot, so long animations are cheap. After 2000 frames, the tracer records only the final frame, and the narration says that the animation skips the later steps.

**The visualizer** has two layers. `Player`, `TreeView`, `Screen` and `Canvas` have no terminal I/O, so the tests use them directly. `Player#press` is the only way to change the state; `App` and `--render` both use it. `App` is one loop on one thread: `IO.select` waits for a key, a resize (`SIGWINCH` writes to a pipe) or the time of the next animation step. When you quit, or when an error occurs, `App` restores the terminal.

## Design decisions

The assignment does not specify these points:

- **Union.** The calculator visits Y in pre-order and inserts each value into X. X keeps its nodes and its shape, and the new values go to the correct places.
- **Intersection.** The calculator visits X in pre-order, searches Y for each value, and inserts each value that Y has into a new tree. The new tree becomes X. Pre-order keeps the shape of X for the elements that stay.
- **Deep copy.** `c` makes new nodes for Y in the same shape as X. A later change to X does not change Y.
- **Values.** A set holds integers and decimals. `X 1,abc` is an error, and X does not change. The original program changed `abc` to 0.
- **Duplicates.** `X 1,2,1` gives {1, 2} and a note. `a 5` when X has 5 gives a note, and X does not change.
- **Size.** A set holds at most 1000 elements. A command that makes a larger set is an error, and the sets do not change. This limit keeps the animation and the recursive copy fast. The calculator stops the count at the first value past the limit, so a very long list gives the error at once.
- **The lambda output.** `l` prints one list with a result for each element, in ascending order.

## Bugs of the original version

The original `main.rb` (at the git tag `college-original`) had these defects. Each one was verified with Ruby 4.0:

1. **The first command crashed the program.** The sets started as `nil`, not as empty sets. After the first command, the program printed Y with `set_Y.in_order`, and Ruby raised `NoMethodError: undefined method 'in_order' for nil`. Thus, a session could never execute a second command.
2. **The commands did not follow the specification.** The program showed a numbered menu (1 to 11) and asked for the arguments on separate lines. The specification asks for commands such as `X 5,3,8`, `a 7` and `l aString`.
3. **Union did not work.** `u` executed `set_X.in_order | set_Y.in_order`. `in_order` printed the values and returned `nil`, so the expression was `nil | nil` (false), and X did not change. `BST#|` called `|` on two `Node` objects, which have no such method.
4. **Intersection did not work,** for the same reason: `nil & nil`.
5. **The lambda command ignored the user.** It always used the fixed text `-> (value){value + 2}`, and it called the lambda once with the `nil` result of `in_order`. Ruby raised `NoMethodError` for `nil + 2`. It also used `eval` with no protection.
6. **The deep copy was not recursive.** `c` used `Marshal.load(Marshal.dump(set_X))`. The specification asks for a recursive deep copy.
7. **Bad numbers became 0.** The program used `to_i`, so `abc` became 0 and `2.5` became 2.
8. **Building a set printed it.** `BST#traverse` printed the values as a side effect, and the program also printed `BST` (`p set_X.class`) after `X`.

## Tests

`bundle exec rake test` runs 124 tests. They examine:

- the BST: the shape after each insert, duplicates, 2 and 2.0, values that are not numbers, the events of insert and search, pre-order, a chain of 3000 nodes, and the deep copy (new nodes, the same shape, the order of the recursion);
- the parser: each command, the separators, the number formats and the bad input;
- each command of the calculator, with empty sets and the size limit;
- the lambda: the accepted forms, about 30 rejected texts (`system`, backticks, `send`, `eval`, blocks, loops, constants and more), errors in a call, the child process and its time limit;
- the frames of each command: the narration, the marks, the shared snapshots and the frame budget;
- the tree drawing: the exact text of small trees, the compact labels, scrolling, the markers, the scroll limits, and that the view never cuts a number;
- the player keys, and the screen at sizes from 80 × 22 to 160 × 48: each line fits;
- the classic mode with piped input, and the options.

## History

This project started as a course project for object-oriented programming in Ruby. The original code is at the git tag `college-original`. This version is a new implementation. It keeps the specification of the assignment: three sets in hand-written binary search trees, the commands, the recursive deep copy and the lambda command. It corrects the defects of the original.
