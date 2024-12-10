#pragma once

#include <libdwarf.h>

#include <cppdwarf/details/attribute_t.hpp>
#include <cppdwarf/details/exceptions.hpp>
#include <cppdwarf/details/form.hpp>

namespace cppdwarf {

class attribute {
public:
    explicit attribute(Dwarf_Debug dbg, Dwarf_Attribute attr) : dbg_(dbg), attr_(attr) {}

    attribute(const attribute &) = delete;
    attribute &operator=(const attribute &) = delete;

    attribute(attribute &&other) noexcept : dbg_(other.dbg_), attr_(other.attr_)
    {
        other.attr_ = nullptr;
    }

    attribute &operator=(attribute &&other) noexcept
    {
        if (this != &other) {
            dbg_ = other.dbg_;
            attr_ = other.attr_;
            other.attr_ = nullptr;
        }
        return *this;
    }

    ~attribute() = default;

    [[nodiscard]] const char *name() const
    {
        int res = 0;
        Dwarf_Half attrnum = static_cast<Dwarf_Half>(type());
        const char *attrname = nullptr;
        res = dwarf_get_AT_name(attrnum, &attrname);
        if (res != DW_DLV_OK) {
            throw other_error("dwarf_get_AT_name failed!");
        }
        return attrname;
    }

    [[nodiscard]] attribute_t type() const
    {
        Dwarf_Error error = nullptr;
        Dwarf_Half attr_num = 0;
        int res = dwarf_whatattr(attr_, &attr_num, &error);
        if (res != DW_DLV_OK) {
            throw other_error("dwarf_whatattr failed!");
        }
        return static_cast<attribute_t>(attr_num);
    }

    [[nodiscard]] form form() const
    {
        Dwarf_Error error = nullptr;
        Dwarf_Half final_form = 0;
        int res = dwarf_whatform(attr_, &final_form, &error);
        if (res != DW_DLV_OK) {
            throw other_error("dwarf_whatform failed!");
        }
        return static_cast<cppdwarf::form>(final_form);
    }

    [[nodiscard]] bool is_string() const noexcept
    {
        switch (form()) {
        case form::string:
        case form::GNU_strp_alt:
        case form::GNU_str_index:
        case form::strx:
        case form::strx1:
        case form::strx2:
        case form::strx3:
        case form::strx4:
        case form::strp:
            return true;
        default:
            break;
        }
        return false;
    }

    template <typename T>
    T get() const
    {
        static_assert(sizeof(T) == 0, "unsupported type for attribute::get()");
        throw type_error("unsupported type for attribute::get()");
    }

private:
    Dwarf_Debug dbg_ = nullptr;
    Dwarf_Attribute attr_;
};

template <>
[[nodiscard]] inline std::string attribute::get<std::string>() const
{
    char *value = nullptr;
    Dwarf_Error error = nullptr;
    if (dwarf_formstring(attr_, &value, &error) != DW_DLV_OK) {
        throw type_error("dwarf_formstring failed!");
    }
    return value;
}

inline std::ostream &operator<<(std::ostream &os, const attribute &attr)
{
    os << "attr: " << attr.name() << ", form: " << attr.form();
    if (attr.is_string()) {
        os << ", value: " << attr.get<std::string>();
    }
    return os;
}

} // namespace cppdwarf
