#pragma once

#include <libdwarf.h>

#include <cppdwarf/details/attribute.hpp>
#include <cppdwarf/details/attribute_list.hpp>
#include <cppdwarf/details/tag.hpp>

namespace cppdwarf {

class die {
public:
    explicit die(Dwarf_Debug dbg, Dwarf_Die die) : dbg_(dbg), die_(die, dwarf_dealloc_die)
    {
        attributes_ = std::make_unique<attribute_list>(dbg_, die_.get());
    }

    die(const die &) = delete;
    die &operator=(const die &) = delete;

    die(die &&other) noexcept : dbg_(other.dbg_), die_(std::move(other.die_)), attributes_(std::move(other.attributes_))
    {
        other.die_ = nullptr;
        other.attributes_ = nullptr;
    }

    die &operator=(die &&other) noexcept
    {
        if (this != &other) {
            dbg_ = other.dbg_;
            die_ = std::move(other.die_);
            other.die_ = nullptr;
            attributes_ = std::move(other.attributes_);
            other.attributes_ = nullptr;
        }
        return *this;
    }

    ~die() = default;

    [[nodiscard]] tag tag() const
    {
        Dwarf_Half tag;
        Dwarf_Error error = nullptr;
        int res = dwarf_tag(die_.get(), &tag, &error);
        if (res == DW_DLV_ERROR) {
            throw other_error("dwarf_tag failed!");
        }
        return static_cast<cppdwarf::tag>(tag);
    }

    [[nodiscard]] attribute_list &attributes() const
    {
        return *attributes_;
    }

private:
    Dwarf_Debug dbg_ = nullptr;
    std::unique_ptr<Dwarf_Die_s, decltype(&dwarf_dealloc_die)> die_;
    std::unique_ptr<attribute_list> attributes_;
};

} // namespace cppdwarf
