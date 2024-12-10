#pragma once

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace posixpath {

static constexpr char sep = '/';

// Function to split a path into components
inline std::vector<std::string> split(const std::string &path)
{
    std::vector<std::string> components;
    std::istringstream stream(path);
    std::string segment;
    while (std::getline(stream, segment, sep)) {
        components.push_back(segment);
    }
    return components;
}

// Function to join components into a path using variadic parameters
inline std::string join(const std::vector<std::string> &paths)
{
    std::ostringstream joined;
    for (size_t i = 0; i < paths.size(); ++i) {
        if (i > 0) {
            joined << '/'; // Add a separator between components
        }
        joined << paths[i];
    }
    return joined.str();
}

// Variadic join function
template <typename... Args>
std::string join(Args... paths)
{
    const std::vector<std::string> components = {paths...};
    return join(components);
}

// Function to find the common path
inline std::string commonpath(const std::vector<std::string> &paths)
{
    if (paths.empty()) {
        return "";
    }

    // Split all paths into components
    std::vector<std::vector<std::string>> split_paths;
    split_paths.reserve(paths.size());
    for (const auto &path : paths) {
        split_paths.push_back(split(path));
    }

    // Find the common prefix of all split paths
    std::vector<std::string> common;
    for (size_t i = 0; i < split_paths[0].size(); ++i) {
        std::string current = split_paths[0][i];
        for (size_t j = 1; j < split_paths.size(); ++j) {
            if (i >= split_paths[j].size() || split_paths[j][i] != current) {
                return join(common);
            }
        }
        common.push_back(current);
    }

    return join(common);
}

} // namespace posixpath
