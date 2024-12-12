#pragma once

#include <cppdwarf/details/exceptions.hpp>

namespace cppdwarf {

class attribute_list {
public:
    attribute_list(Dwarf_Debug dbg, Dwarf_Die die) : dbg_(dbg), die_(die)
    {
        Dwarf_Error error = nullptr;
        int res = dwarf_attrlist(die, &attributes_, &attr_count_, &error);
        if (res != DW_DLV_OK) {
            attributes_ = nullptr;
            attr_count_ = 0;
        }
    }

    ~attribute_list()
    {
        if (attributes_) {
            for (int i = 0; i < attr_count_; ++i) {
                dwarf_dealloc_attribute(attributes_[i]);
            }
            dwarf_dealloc(dbg_, attributes_, DW_DLA_LIST);
            attributes_ = nullptr;
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

        iterator_base(Dwarf_Debug dbg, Dwarf_Attribute *attributes, Dwarf_Signed index, Dwarf_Signed count)
            : dbg_(dbg), attributes_(attributes), index_(index), count_(count)
        {
        }

        iterator_base &operator++()
        {
            ++index_;
            return *this;
        }

        bool operator==(const iterator_base &other) const
        {
            return index_ == other.index_ && attributes_ == other.attributes_;
        }

        bool operator!=(const iterator_base &other) const
        {
            return !(*this == other);
        }

        reference operator*() const
        {
            if (index_ >= count_) {
                throw out_of_range("index is out of range");
            }
            return value_type(dbg_, attributes_[index_]);
        }

        std::unique_ptr<value_type> operator->() const
        {
            if (index_ >= count_) {
                throw out_of_range("index is out of range");
            }
            return std::make_unique<value_type>(dbg_, attributes_[index_]);
        }

    private:
        Dwarf_Debug dbg_;
        Dwarf_Attribute *attributes_ = nullptr;
        Dwarf_Signed index_ = 0;
        Dwarf_Signed count_ = 0;
    };

public:
    using iterator = iterator_base<attribute>;
    using const_iterator = iterator_base<const attribute>;

    iterator begin()
    {
        return {dbg_, attributes_, 0, attr_count_};
    }

    iterator end()
    {
        return {dbg_, attributes_, attr_count_, attr_count_};
    }

    [[nodiscard]] const_iterator begin() const
    {
        return {dbg_, attributes_, 0, attr_count_};
    }

    [[nodiscard]] const_iterator end() const
    {
        return {dbg_, attributes_, attr_count_, attr_count_};
    }

    attribute operator[](std::size_t index) const
    {
        return attribute(dbg_, attributes_[index]);
    }

    [[nodiscard]] attribute at(std::size_t index) const
    {
        if (index >= attr_count_) {
            throw out_of_range("index is out of range");
        }
        return attribute(dbg_, attributes_[index]);
    }

    [[nodiscard]] const_iterator::reference at(attribute_t type) const
    {
        const auto it = find(type);
        if (it == end()) {
            throw out_of_range("attribute not found");
        }
        return *it;
    }

    [[nodiscard]] const_iterator find(attribute_t type) const
    {
        for (auto it = begin(); it != end(); ++it) {
            if (it->type() == type) {
                return it;
            }
        }
        return end();
    }

    [[nodiscard]] bool contains(attribute_t type) const
    {
        return find(type) != end();
    }

    [[nodiscard]] std::size_t size() const
    {
        return attr_count_;
    }

    [[nodiscard]] bool empty() const
    {
        return attr_count_ == 0;
    }

private:
    Dwarf_Debug dbg_;
    Dwarf_Die die_;
    Dwarf_Attribute *attributes_ = nullptr;
    Dwarf_Signed attr_count_ = 0;
};

} // namespace cppdwarf
