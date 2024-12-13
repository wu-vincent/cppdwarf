#pragma once

#include <dwarf.h>
#include <libdwarf.h>

#include <ostream>

namespace cppdwarf {

enum class access {
    public_ = DW_ACCESS_public,
    protected_ = DW_ACCESS_protected,
    private_ = DW_ACCESS_private,
};

inline std::ostream &operator<<(std::ostream &os, access a)
{
    const char *name = nullptr;
    int res = dwarf_get_ACCESS_name(static_cast<Dwarf_Half>(a), &name);
    if (res != DW_DLV_OK) {
        os << "<bogus access>";
    }
    else {
        os << name;
    }
    return os;
}

} // namespace cppdwarf
