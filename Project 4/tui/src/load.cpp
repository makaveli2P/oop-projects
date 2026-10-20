#include "load.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace sal::tui {

LoadedFile load_file(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream in{path, std::ios::binary};
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return LoadedFile{std::filesystem::path{path}.filename().string(), sal::assemble_file(path),
                      std::move(lines)};
}

}  // namespace sal::tui
