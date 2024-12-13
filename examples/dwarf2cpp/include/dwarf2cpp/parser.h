#pragma once

#include <utility>

#include <cppdwarf/cppdwarf.hpp>
#include <spdlog/fmt/ranges.h>

#include "dwarf2cpp/posixpath.hpp"
#include "dwarf2cpp/source_file.h"

namespace dw = cppdwarf;

class debug_parser {
public:
    explicit debug_parser(dw::debug &dbg) : dbg_(dbg) {}

    void parse();

    [[nodiscard]] std::string base_dir() const
    {
        return base_dir_;
    }

    [[nodiscard]] const std::unordered_map<std::string, source_file> &result() const
    {
        return result_;
    }

private:
    friend class cu_parser;

    void add_entry(std::string file, std::size_t line, std::unique_ptr<entry> entry)
    {
        file = posixpath::normpath(file);
        result_[file].add(line, std::move(entry));
    }

    dw::debug &dbg_;
    std::string base_dir_;
    std::unordered_map<std::string, source_file> result_{};
};

class cu_parser {
    using namespace_list = std::vector<std::string>;

public:
    cu_parser(dw::compilation_unit &cu, debug_parser &dbg_parser);
    void parse();

    [[nodiscard]] std::string name() const
    {
        return name_;
    }

    [[nodiscard]] std::string base_dir() const
    {
        return base_dir_;
    }

    [[nodiscard]] std::string get_type(const std::size_t offset) const
    {
        return type_info_.at(offset);
    }

    void parse_children(const dw::die &die, namespace_list &namespaces);
    void parse_namespace(const dw::die &die, namespace_list &namespaces);
    // parse a non-member type, for member types, see parse_member_type
    void parse_type(const dw::die &die, const namespace_list &namespaces);
    // parse a non-member function, for member functions, see parse_member_function
    void parse_function(const dw::die &die, const namespace_list &namespaces);

    void add_entry(std::string file, std::size_t line, std::unique_ptr<entry> entry) const;
    static std::string get_qualified_name(const namespace_list &namespaces, const std::string &name)
    {
        std::string qualified_name;
        for (const auto &ns : namespaces) {
            if (ns.empty()) {
                continue;
            }
            if (!qualified_name.empty()) {
                qualified_name += "::";
            }
            qualified_name += ns;
        }
        if (!qualified_name.empty()) {
            qualified_name += "::";
        }
        qualified_name += name;
        return qualified_name;
    }

private:
    dw::compilation_unit &cu_;
    std::string name_;
    std::string base_dir_;
    std::vector<std::string> src_files_;
    std::unordered_map<std::size_t, std::string> type_info_{};
    debug_parser &dbg_parser_;
};
