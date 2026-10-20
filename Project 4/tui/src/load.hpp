// load.hpp - reads a SAL file for the visualizer: the program or the errors,
// and the source lines for the error screen.
#pragma once

#include <string>
#include <vector>

#include "sal/assembler.hpp"

namespace sal::tui {

struct LoadedFile {
    std::string file_name;           ///< The file name without the folders.
    sal::AssembleResult result;      ///< The program or the errors.
    std::vector<std::string> lines;  ///< The source lines, without line ends.
};

[[nodiscard]] LoadedFile load_file(const std::string& path);

}  // namespace sal::tui
