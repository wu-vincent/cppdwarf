#pragma once

#include <dwarf.h>

namespace cppdwarf {
enum class virtuality {
    none = DW_VIRTUALITY_none,
    virtual_ = DW_VIRTUALITY_virtual,
    pure_virtual = DW_VIRTUALITY_pure_virtual,
};

inline std::ostream &operator<<(std::ostream &os, virtuality v)
{
    const char *name = nullptr;
    int res = dwarf_get_VIRTUALITY_name(static_cast<Dwarf_Half>(v), &name);
    if (res != DW_DLV_OK) {
        os << "<bogus virtuality>";
    }
    else {
        os << name;
    }
    return os;
}

} // namespace cppdwarf
