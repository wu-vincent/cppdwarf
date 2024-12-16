#pragma once

#include <utility>

#include <cppdwarf/cppdwarf.hpp>
#include <spdlog/fmt/ostr.h>
#include <spdlog/fmt/ranges.h>

#include "dwarf2cpp/posixpath.hpp"
#include "dwarf2cpp/source_file.h"

namespace dw = cppdwarf;

template <>
struct fmt::formatter<dw::die> : ostream_formatter {};

class debug_parser {
public:
    struct result {
        std::string base_dir;
        std::unordered_map<std::string, source_file> files;
    };

    explicit debug_parser(dw::debug &dbg) : dbg_(dbg) {}

    const result &parse();

private:
    friend class cu_parser;

    void add_entry(std::string file, std::size_t line, std::unique_ptr<entry> entry)
    {
        file = posixpath::normpath(file);
        result_.files[file].add(line, std::move(entry));
    }

    dw::debug &dbg_;
    result result_;
};

class cu_parser {
    using namespace_list = std::vector<std::string>;

public:
    cu_parser(dw::compilation_unit &cu, debug_parser &dbg_parser);
    void parse();

    [[nodiscard]] type_t get_type(const dw::die &die);

private:
    void parse_types(const dw::die &die, namespace_list &parents);

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
    std::vector<std::string> src_files_;
    std::unordered_map<std::size_t, type_t> known_types_{};
    debug_parser &dbg_parser_;
};
