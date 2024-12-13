#include "dwarf2cpp/source_file.h"

#include <sstream>

void source_file::add(std::size_t line, std::unique_ptr<entry> new_entry)
{
    lines_[line] = std::move(new_entry);
}

std::string source_file::to_source() const
{
    std::stringstream ss;
    std::vector<std::string> prev_namespaces;

    for (const auto &[line, entry] : lines_) {

        // Get current namespaces
        const auto &current_namespaces = entry->namespaces();

        // Find the point of divergence between previous and current namespaces
        size_t level = 0;
        while (level < prev_namespaces.size() && level < current_namespaces.size() &&
               prev_namespaces[level] == current_namespaces[level]) {
            ++level;
        }

        // Close namespaces that are no longer needed
        for (size_t i = prev_namespaces.size(); i > level; --i) {
            ss << "} // namespace " << prev_namespaces[i - 1] << "\n";
        }

        // Open new namespaces
        for (size_t i = level; i < current_namespaces.size(); ++i) {
            ss << "namespace " << current_namespaces[i] << " {\n";
        }

        // Update the tracked namespaces
        prev_namespaces = current_namespaces;

        // Print the line and entry's source code
        ss << "// Line " << line << "\n";
        ss << entry->to_source() << "\n\n";
    }

    // Close any remaining open namespaces
    for (auto it = prev_namespaces.rbegin(); it != prev_namespaces.rend(); ++it) {
        ss << "} // namespace " << *it << "\n";
    }

    return ss.str();
}

std::ostream &operator<<(std::ostream &os, const source_file &sf)
{
    os << sf.to_source() << "\n";
    return os;
}
