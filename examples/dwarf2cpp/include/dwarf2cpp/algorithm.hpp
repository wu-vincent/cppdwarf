#pragma once

#include <algorithm>
#include <string>

inline bool starts_with(const std::string &str, const std::string &prefix)
{
    if (prefix.size() > str.size()) {
        return false;
    }
    return str.compare(0, prefix.size(), prefix) == 0;
}
