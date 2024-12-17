#pragma once

#include <functional>
#include <list>

namespace cppdwarf {

class attribute_list {
    using handle_t = std::unique_ptr<Dwarf_Attribute, std::function<void(Dwarf_Attribute *)>>;

public:
    attribute_list(Dwarf_Debug dbg, Dwarf_Die die) : dbg_(dbg), die_(die)
    {
        Dwarf_Error error = nullptr;
        Dwarf_Attribute *attr_list = nullptr;
        Dwarf_Signed attr_count = 0;
        int res = dwarf_attrlist(die, &attr_list, &attr_count, &error);
        if (res == DW_DLV_OK) {
            handle_ = handle_t(attr_list, [&](auto *list) { dwarf_dealloc(dbg_, list, DW_DLA_LIST); });
            for (auto i = 0; i < attr_count; i++) {
                auto &ref = attributes_.emplace_back(std::make_unique<attribute>(dbg, attr_list[i]));
                attributes_map_.emplace(ref->type(), ref.get());
            }
        }
    }

    auto begin()
    {
        return attributes_.begin();
    }

    auto end()
    {
        return attributes_.end();
    }

    [[nodiscard]] auto begin() const
    {
        return attributes_.begin();
    }

    [[nodiscard]] auto end() const
    {
        return attributes_.end();
    }

    auto &operator[](std::size_t index) const
    {
        return attributes_[index];
    }

    [[nodiscard]] auto &at(std::size_t index) const
    {
        return attributes_.at(index);
    }

    [[nodiscard]] auto &at(attribute_t type) const
    {
        return attributes_map_.at(type);
    }

    [[nodiscard]] auto find(attribute_t type) const
    {
        return attributes_map_.find(type);
    }

    [[nodiscard]] bool contains(attribute_t type) const
    {
        return attributes_map_.find(type) != attributes_map_.end();
    }

    [[nodiscard]] std::size_t size() const
    {
        return attributes_.size();
    }

    [[nodiscard]] bool empty() const
    {
        return attributes_.empty();
    }

private:
    Dwarf_Debug dbg_;
    Dwarf_Die die_;
    handle_t handle_ = handle_t(nullptr, [](auto *) {});
    std::vector<std::unique_ptr<attribute>> attributes_;
    std::unordered_map<attribute_t, attribute *> attributes_map_;
};

} // namespace cppdwarf
