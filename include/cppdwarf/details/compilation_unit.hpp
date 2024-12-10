#pragma once

namespace cppdwarf {

class compilation_unit {
public:
    compilation_unit(std::size_t cu_header_length, int version_stamp, std::size_t abbrev_offset, int address_size)
        : cu_header_length_(cu_header_length), version_stamp_(version_stamp), abbrev_offset_(abbrev_offset),
          address_size_(address_size)
    {
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
    std::size_t cu_header_length_;
    int version_stamp_;
    std::size_t abbrev_offset_;
    int address_size_;
};

} // namespace cppdwarf
