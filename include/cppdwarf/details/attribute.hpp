#pragma once

#include <cppdwarf/details/exceptions.hpp>

namespace cppdwarf {

class attribute {
public:
    explicit attribute(Dwarf_Debug dbg, Dwarf_Attribute attr) : dbg_(dbg), attr_(attr, dwarf_dealloc_attribute) {}

    attribute(const attribute &) = delete;
    attribute &operator=(const attribute &) = delete;

    attribute(attribute &&other) noexcept : dbg_(other.dbg_), attr_(std::move(other.attr_))
    {
        other.attr_ = nullptr;
    }

    attribute &operator=(attribute &&other) noexcept
    {
        if (this != &other) {
            dbg_ = other.dbg_;
            attr_ = std::move(other.attr_);
            other.attr_ = nullptr;
        }
        return *this;
    }

    ~attribute() = default;

    [[nodiscard]] const char *name() const
    {
        int res = 0;
        Dwarf_Error error = nullptr;
        Dwarf_Half attrnum = 0;
        res = dwarf_whatattr(attr_.get(), &attrnum, &error);
        if (res != DW_DLV_OK) {
            throw other_error("dwarf_whatattr failed!");
        }

        const char *attrname = nullptr;
        res = dwarf_get_AT_name(attrnum, &attrname);
        if (res != DW_DLV_OK) {
            throw other_error("dwarf_get_AT_name failed!");
        }
        return attrname;
    }

    template <typename T>
    T get() const
    {
        static_assert(sizeof(T) == 0, "unsupported type for attribute::get()");
        throw type_error("unsupported type for attribute::get()");
    }

private:
    Dwarf_Debug dbg_ = nullptr;
    std::unique_ptr<Dwarf_Attribute_s, decltype(&dwarf_dealloc_attribute)> attr_;
};

template <>
[[nodiscard]] inline std::string attribute::get<std::string>() const
{
    char *value = nullptr;
    Dwarf_Error error = nullptr;
    if (dwarf_formstring(attr_.get(), &value, &error) != DW_DLV_OK) {
        throw type_error("dwarf_formstring failed!");
    }
    return value;
}

} // namespace cppdwarf
