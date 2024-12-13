#pragma once

#include <map>
#include <string>

#include "dwarf2cpp/entry.h"

class source_file {
public:
    void add(std::size_t line, std::unique_ptr<entry> new_entry);
    [[nodiscard]] std::string to_source() const;
    friend std::ostream &operator<<(std::ostream &os, const source_file &sf);

private:
    std::map<std::size_t, std::unique_ptr<entry>> lines_;
};
