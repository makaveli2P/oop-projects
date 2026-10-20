// classic.h - the classic command loop of the course project.
#pragma once

#include <iosfwd>

namespace strset {

/// Runs the command loop: it reads a command, executes it, and prints S1
/// and S2. "q" or the end of the input stops the loop. Returns the process
/// exit code (0).
int run_classic(std::istream& in, std::ostream& out);

/// Prints the list of commands.
void print_commands(std::ostream& out);

}  // namespace strset
