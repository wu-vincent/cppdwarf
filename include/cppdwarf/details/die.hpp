#pragma once

namespace cppdwarf {

class die {
public:
    explicit die(Dwarf_Debug dbg, Dwarf_Die die) : dbg_(dbg), die_(die, dwarf_dealloc_die) {}

    die(const die &) = delete;
    die &operator=(const die &) = delete;

    die(die &&other) noexcept : dbg_(other.dbg_), die_(std::move(other.die_))
    {
        other.die_ = nullptr;
    }

    die &operator=(die &&other) noexcept
    {
        if (this != &other) {
            dbg_ = other.dbg_;
            die_ = std::move(other.die_);
            other.die_ = nullptr;
        }
        return *this;
    }

    ~die() = default;

private:
    Dwarf_Debug dbg_ = nullptr;
    std::unique_ptr<Dwarf_Die_s, decltype(&dwarf_dealloc_die)> die_;
};

} // namespace cppdwarf
