#pragma once

#include <libdwarf.h>

#include <iomanip>

#include <cppdwarf/details/attribute.hpp>
#include <cppdwarf/details/attribute_list.hpp>
#include <cppdwarf/details/tag.hpp>

namespace cppdwarf {

class die {
public:
    explicit die(Dwarf_Debug dbg, Dwarf_Die die) : dbg_(dbg), die_(die)
    {
        attributes_ = std::make_unique<attribute_list>(dbg_, die_);
    }

    die(const die &) = delete;
    die &operator=(const die &) = delete;

    die(die &&other) noexcept : dbg_(other.dbg_), die_(other.die_), attributes_(std::move(other.attributes_))
    {
        other.die_ = nullptr;
        other.attributes_ = nullptr;
    }

    die &operator=(die &&other) noexcept
    {
        if (this != &other) {
            dbg_ = other.dbg_;
            if (die_) {
                dwarf_dealloc_die(die_);
                die_ = nullptr;
            }
            die_ = other.die_;
            other.die_ = nullptr;
            attributes_ = std::move(other.attributes_);
            other.attributes_ = nullptr;
        }
        return *this;
    }

    ~die()
    {
        if (die_) {
            dwarf_dealloc_die(die_);
            die_ = nullptr;
        }
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

        iterator_base(Dwarf_Debug dbg, Dwarf_Die parent_die, bool is_end = false) : dbg_(dbg), is_end_(is_end)
        {
            if (!is_end) {
                Dwarf_Die child = nullptr;
                Dwarf_Error error = nullptr;
                if (dwarf_child(parent_die, &child, &error) != DW_DLV_OK) {
                    current_die_ = nullptr;
                    is_end_ = true;
                    return;
                }
                current_die_ = std::make_unique<die>(dbg_, child);
            }
        }

        iterator_base &operator++()
        {
            if (is_end_) {
                return *this;
            }

            Dwarf_Error error = nullptr;
            Dwarf_Die next_die = nullptr;
            int result = dwarf_siblingof_c(current_die_->die_, &next_die, &error);
            if (result == DW_DLV_NO_ENTRY) {
                is_end_ = true;
                current_die_ = nullptr;
            }
            else if (result != DW_DLV_OK) {
                throw invalid_iterator("dwarf_siblingof_c failed!");
            }
            else {
                current_die_ = std::make_unique<die>(dbg_, next_die);
            }

            return *this;
        }

        bool operator==(const iterator_base &other) const
        {
            return is_end_ == other.is_end_ && current_die_ == other.current_die_;
        }

        bool operator!=(const iterator_base &other) const
        {
            return !(*this == other);
        }

        reference operator*() const
        {
            if (is_end_) {
                throw invalid_iterator("end iterator");
            }
            return *current_die_;
        }

    private:
        Dwarf_Debug dbg_ = nullptr;
        std::unique_ptr<die> current_die_ = nullptr;
        bool is_end_ = false;
    };

public:
    using iterator = iterator_base<die>;
    using const_iterator = iterator_base<const die>;

    iterator begin()
    {
        return {dbg_, die_};
    }

    iterator end()
    {
        return {dbg_, nullptr, true};
    }

    [[nodiscard]] const_iterator begin() const
    {
        return {dbg_, die_};
    }
    [[nodiscard]] const_iterator end() const
    {
        return {dbg_, nullptr, true};
    }

    [[nodiscard]] tag tag() const
    {
        Dwarf_Half tag;
        Dwarf_Error error = nullptr;
        int res = dwarf_tag(die_, &tag, &error);
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
    Dwarf_Die die_;
    std::unique_ptr<attribute_list> attributes_;
};

} // namespace cppdwarf
