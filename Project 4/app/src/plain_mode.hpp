// plain_mode.hpp - the classic text mode: the s / a / q command loop.
#pragma once

#include <iosfwd>
#include <optional>
#include <string>

namespace ali {

/// Runs the classic command loop. If `file` is empty, the function asks for
/// a file name on `in`. Returns the process exit code.
int run_plain(const std::optional<std::string>& file, std::istream& in, std::ostream& out,
              std::ostream& err);

}  // namespace ali
