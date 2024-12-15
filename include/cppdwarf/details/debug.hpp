#pragma once

#include <libdwarf.h>

#include <string>

#include <cppdwarf/details/compilation_unit.hpp>
#include <cppdwarf/details/exceptions.hpp>

namespace cppdwarf {

class debug {
public:
    explicit debug(const std::string &file_path)
    {
        Dwarf_Error error = nullptr;
        Dwarf_Debug dbg = nullptr;
        int res = dwarf_init_path(file_path.c_str(), nullptr, 0, DW_GROUPNUMBER_ANY, nullptr, nullptr, &dbg, &error);
        if (res != DW_DLV_OK) {
            std::string msg = error ? dwarf_errmsg(error) : "";
            dwarf_dealloc_error(dbg, error);
            dwarf_finish(dbg);
            throw init_error("dwarf_init_path failed! " + msg);
        }
        dbg_ = dbg;
    }

    // Destructor ensures proper cleanup of Dwarf_Debug
    ~debug()
    {
        if (dbg_) {
            dwarf_finish(dbg_);
        }
    }

    debug(const debug &) = delete;
    debug &operator=(const debug &) = delete;

    debug(debug &&other) noexcept : dbg_(other.dbg_)
    {
        other.dbg_ = nullptr;
    }

    debug &operator=(debug &&other) noexcept
    {
        if (this != &other) {
            if (dbg_) {
                dwarf_finish(dbg_);
                dbg_ = nullptr;
            }
            dbg_ = other.dbg_;
            other.dbg_ = nullptr;
        }
        return *this;
    }

public:
    using iterator = compilation_unit_list::iterator;
    using const_iterator = compilation_unit_list::const_iterator;

    iterator begin()
    {
        return compilation_units().begin();
    }

    iterator end()
    {
        return compilation_units().end();
    }

    [[nodiscard]] const_iterator begin() const
    {
        return compilation_units().cbegin();
    }

    [[nodiscard]] const_iterator end() const
    {
        return compilation_units().cend();
    }

    [[nodiscard]] compilation_unit_list compilation_units() const
    {
        return compilation_unit_list(dbg_, true);
    }

    [[nodiscard]] compilation_unit_list type_units() const
    {
        return compilation_unit_list(dbg_, false);
    }

private:
    Dwarf_Debug dbg_ = nullptr;
};

} // namespace cppdwarf
