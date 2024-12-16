#pragma once

#include <cppdwarf/details/die.hpp>

namespace cppdwarf {

class compilation_unit {
public:
    compilation_unit(Dwarf_Debug dbg, Dwarf_Die die, bool is_info, std::size_t cu_header_length, int version_stamp,
                     std::size_t abbrev_offset, int address_size)
        : die_(dbg, die, is_info), is_info_(is_info), cu_header_length_(cu_header_length),
          version_stamp_(version_stamp), abbrev_offset_(abbrev_offset), address_size_(address_size)
    {
    }

    [[nodiscard]] const die &die() const
    {
        return die_;
    }

    [[nodiscard]] std::size_t header_length() const
    {
        return cu_header_length_;
    }

    [[nodiscard]] int version() const
    {
        return version_stamp_;
    }

    [[nodiscard]] std::size_t abbrev_offset() const
    {
        return abbrev_offset_;
    }

    [[nodiscard]] int address_size() const
    {
        return address_size_;
    }

private:
    cppdwarf::die die_;
    bool is_info_;
    std::size_t cu_header_length_;
    int version_stamp_;
    std::size_t abbrev_offset_;
    int address_size_;
};

} // namespace cppdwarf
