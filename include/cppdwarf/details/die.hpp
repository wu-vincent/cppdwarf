#pragma once

#include <libdwarf.h>

#include <iomanip>
#include <vector>

#include <cppdwarf/details/attribute.hpp>
#include <cppdwarf/details/attribute_list.hpp>
#include <cppdwarf/details/enums.hpp>

namespace cppdwarf {

class die {
public:
    explicit die(Dwarf_Debug dbg, Dwarf_Die die, bool is_info)
        : dbg_(dbg), die_(die, dwarf_dealloc_die), is_info_(is_info)
    {
        attributes_ = std::make_unique<attribute_list>(dbg_, die_.get());
    }

    die(const die &) = delete;
    die &operator=(const die &) = delete;

    die(die &&other) = default;
    die &operator=(die &&other) = default;

    [[nodiscard]] std::size_t offset() const
    {
        Dwarf_Off offset = 0;
        Dwarf_Error error = nullptr;
        int res = dwarf_dieoffset(die_.get(), &offset, &error);
        if (res != DW_DLV_OK) {
            throw other_error("dwarf_dieoffset failed!");
        }
        return offset;
    }

    [[nodiscard]] bool is_info() const
    {
        return is_info_;
    }

private:
    template <typename T>
    class iterator_base {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using reference = T &;

        iterator_base(Dwarf_Debug dbg, Dwarf_Die parent_die, bool is_info) : dbg_(dbg), is_info_(is_info)
        {
            if (parent_die) {
                Dwarf_Die child = nullptr;
                Dwarf_Error error = nullptr;
                if (dwarf_child(parent_die, &child, &error) != DW_DLV_OK) {
                    current_die_ = nullptr;
                    return;
                }
                current_die_ = std::make_unique<die>(dbg_, child, is_info_);
            }
        }

        iterator_base &operator++()
        {
            if (!current_die_) {
                return *this;
            }
            Dwarf_Error error = nullptr;
            Dwarf_Die next_die = nullptr;
            int result = dwarf_siblingof_c(current_die_->die_.get(), &next_die, &error);
            if (result == DW_DLV_NO_ENTRY) {
                current_die_ = nullptr;
            }
            else if (result != DW_DLV_OK) {
                throw invalid_iterator("dwarf_siblingof_c failed!");
            }
            else {
                current_die_ = std::make_unique<die>(dbg_, next_die, is_info_);
            }
            return *this;
        }

        bool operator==(const iterator_base &other) const
        {
            return current_die_ == other.current_die_;
        }

        bool operator!=(const iterator_base &other) const
        {
            return !(*this == other);
        }

        reference operator*() const
        {
            if (!current_die_) {
                throw invalid_iterator("end iterator");
            }
            return *current_die_;
        }

    private:
        Dwarf_Debug dbg_;
        std::unique_ptr<die> current_die_ = nullptr;
        bool is_info_;
    };

public:
    using iterator = iterator_base<die>;
    using const_iterator = iterator_base<const die>;

    iterator begin()
    {
        return {dbg_, die_.get(), is_info_};
    }

    iterator end()
    {
        return {dbg_, nullptr, is_info_};
    }

    [[nodiscard]] const_iterator begin() const
    {
        return {dbg_, die_.get(), is_info_};
    }
    [[nodiscard]] const_iterator end() const
    {
        return {dbg_, nullptr, is_info_};
    }

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

    [[nodiscard]] const attribute_list &attributes() const
    {
        return *attributes_;
    }

    [[nodiscard]] attribute_list::const_iterator find(attribute_t type) const
    {
        return attributes().find(type);
    }

    [[nodiscard]] bool contains(attribute_t type) const
    {
        return find(type) != attributes().end();
    }

    [[nodiscard]] std::vector<std::string> src_files() const
    {
        char **srcfiles = nullptr;
        Dwarf_Signed count = 0;
        Dwarf_Error error = nullptr;
        int res = dwarf_srcfiles(die_.get(), &srcfiles, &count, &error);
        if (res != DW_DLV_OK) {
            throw other_error("dwarf_srcfiles failed!");
        }

        std::vector<std::string> result;
        for (int i = 0; i < count; ++i) {
            result.emplace_back(srcfiles[i]);
            dwarf_dealloc(dbg_, srcfiles[i], DW_DLA_STRING);
            srcfiles[i] = nullptr;
        }
        dwarf_dealloc(dbg_, srcfiles, DW_DLA_LIST);
        return result;
    }

    friend std::ostream &operator<<(std::ostream &os, const die &d)
    {
        os << "die: " << d.tag() << "\n";
        const auto &attributes = d.attributes();
        for (int i = 0; i < attributes.size(); i++) {
            auto attr = attributes[i];
            if (i > 0) {
                os << "\n";
            }
            os << " [" << std::setw(2) << std::right << i << "] " << attr;
        }
        return os;
    }

private:
    Dwarf_Debug dbg_ = nullptr;
    std::unique_ptr<Dwarf_Die_s, decltype(&dwarf_dealloc_die)> die_;
    bool is_info_;
    std::unique_ptr<attribute_list> attributes_;
};

template <>
inline std::unique_ptr<die> attribute::get<std::unique_ptr<die>>() const
{
    Dwarf_Off offset = 0;
    Dwarf_Bool is_info = 0;
    Dwarf_Error error = nullptr;
    int res = dwarf_global_formref_b(attr_, &offset, &is_info, &error);
    if (res != DW_DLV_OK) {
        throw other_error("dwarf_global_formref failed!");
    }

    Dwarf_Die die;
    res = dwarf_offdie_b(dbg_, offset, is_info, &die, &error);
    if (res != DW_DLV_OK) {
        throw other_error("dwarf_offdie_b failed!");
    }
    return std::make_unique<cppdwarf::die>(dbg_, die, is_info);
}

template <>
inline die attribute::get<die>() const
{
    Dwarf_Off offset = 0;
    Dwarf_Bool is_info = 0;
    Dwarf_Error error = nullptr;
    int res = dwarf_global_formref_b(attr_, &offset, &is_info, &error);
    if (res != DW_DLV_OK) {
        throw other_error("dwarf_global_formref failed!");
    }

    Dwarf_Die die;
    res = dwarf_offdie_b(dbg_, offset, is_info, &die, &error);
    if (res != DW_DLV_OK) {
        throw other_error("dwarf_offdie_b failed!");
    }
    return cppdwarf::die(dbg_, die, is_info);
}

} // namespace cppdwarf
