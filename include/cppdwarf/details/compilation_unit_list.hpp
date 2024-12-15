#pragma once

namespace cppdwarf {

class compilation_unit_list {
public:
    explicit compilation_unit_list(Dwarf_Debug dbg, bool is_info) : dbg_(dbg), is_info_(is_info) {}

private:
    template <typename T>
    class iterator_base {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using reference = T &;

        explicit iterator_base(Dwarf_Debug dbg, bool is_info) : dbg_(dbg), is_info_(is_info), done_(false)
        {
            advance();
        }
        iterator_base() = default; // End iterator
        ~iterator_base() = default;

        iterator_base &operator++()
        {
            advance();
            return *this;
        }

        bool operator==(const iterator_base &other) const
        {
            if (done_ && other.done_) {
                return true;
            }
            return next_cu_header_ == other.next_cu_header_;
        }

        bool operator!=(const iterator_base &other) const
        {
            return !(*this == other);
        }

        reference operator*() const
        {
            if (done_) {
                throw invalid_iterator("end iterator");
            }
            return *cu_;
        }

    private:
        Dwarf_Debug dbg_ = nullptr;
        bool is_info_ = true;
        bool done_ = true;
        Dwarf_Unsigned next_cu_header_ = 0;
        std::unique_ptr<T> cu_;

        void advance()
        {
            if (!dbg_ || done_) {
                done_ = true;
                return;
            }

            Dwarf_Die cu_die = nullptr;
            Dwarf_Unsigned cu_header_length = 0;
            Dwarf_Unsigned abbrev_offset = 0;
            Dwarf_Half version_stamp = 0, address_size = 0, offset_size = 0, extension_size = 0;
            Dwarf_Sig8 signature;
            Dwarf_Unsigned typeoffset = 0;
            Dwarf_Half header_cu_type = 0;
            Dwarf_Error error = nullptr;
            int res = dwarf_next_cu_header_e(dbg_, is_info_, &cu_die, &cu_header_length, &version_stamp, &abbrev_offset,
                                             &address_size, &offset_size, &extension_size, &signature, &typeoffset,
                                             &next_cu_header_, &header_cu_type, &error);
            if (res == DW_DLV_ERROR) {
                throw invalid_iterator("dwarf_next_cu_header_e failed!");
            }
            if (res == DW_DLV_NO_ENTRY) {
                done_ = true; // No more entries
                return;
            }

            cu_ = std::make_unique<T>(dbg_, cu_die, cu_header_length, version_stamp, abbrev_offset, address_size);
        }
    };

public:
    using iterator = iterator_base<compilation_unit>;
    using const_iterator = iterator_base<const compilation_unit>;

    iterator begin()
    {
        return iterator(dbg_, is_info_);
    }

    iterator end()
    {
        return {};
    }

    [[nodiscard]] const_iterator begin() const
    {
        return const_iterator(dbg_, is_info_);
    }

    [[nodiscard]] const_iterator end() const
    {
        return {};
    }

    [[nodiscard]] const_iterator cbegin() const
    {
        return const_iterator(dbg_, is_info_);
    }

    [[nodiscard]] const_iterator cend() const
    {
        return {};
    }

private:
    Dwarf_Debug dbg_;
    bool is_info_;
};

} // namespace cppdwarf
