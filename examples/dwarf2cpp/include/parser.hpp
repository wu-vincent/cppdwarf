#pragma once

#include <posixpath.hpp>

#include <cppdwarf/cppdwarf.hpp>
#include <spdlog/fmt/ranges.h>

namespace dw = cppdwarf;

class cu_parser {
    using namespace_list = std::vector<std::string>;

public:
    explicit cu_parser(dw::compilation_unit &cu) : cu_(cu)
    {
        auto &cu_die = cu.die();
        name_ = cu_die.find(dw::attribute_t::name)->get<std::string>();
        auto comp_dir = cu_die.find(dw::attribute_t::comp_dir)->get<std::string>();
        base_dir_ = posixpath::commonpath({name_, comp_dir});
        src_files_ = cu_die.src_files();
    }

    void parse()
    {
        namespace_list namespaces;
        parse_top_level(cu_.die(), namespaces);
    }

    [[nodiscard]] std::unordered_map<std::string, std::map<int, std::string>> result() const
    {
        return result_;
    }

    [[nodiscard]] std::string name() const
    {
        return name_;
    }

    [[nodiscard]] std::string base_dir() const
    {
        return base_dir_;
    }

private:
    void parse_top_level(const dw::die &die, namespace_list &namespaces) // NOLINT(*-no-recursion)
    {
        for (const auto &child : die) {
            const auto tag = child.tag();
            switch (tag) {
            case dw::tag::namespace_:
                parse_namespace(child, namespaces);
                break;
            case dw::tag::base_type:
            case dw::tag::unspecified_type:
            case dw::tag::typedef_:
            case dw::tag::class_type:
            case dw::tag::structure_type:
            case dw::tag::union_type:
            case dw::tag::enumeration_type:
                parse_type(child, namespaces);
                break;
            case dw::tag::subprogram:
                parse_function(child, namespaces);
                break;
            default:
                break;
            }
        }
    }

    void parse_namespace(const dw::die &die, namespace_list &namespaces) // NOLINT(*-no-recursion)
    {
        std::string name;
        if (auto it = die.find(dw::attribute_t::name); it != die.attributes().end()) {
            name = it->get<std::string>();
        }
        namespaces.push_back(name);
        parse_top_level(die, namespaces);
        namespaces.pop_back();
    }

    void parse_type(const dw::die &die, const namespace_list &namespaces)
    {
        std::string name;
        std::string decl_file;
        int decl_line = 0;
        if (auto it = die.find(dw::attribute_t::name); it != die.attributes().end()) {
            name = it->get<std::string>();
        }
        if (auto it = die.find(dw::attribute_t::decl_file); it != die.attributes().end()) {
            decl_file = posixpath::normpath(src_files_[it->get<int>()]);
        }
        if (auto it = die.find(dw::attribute_t::decl_line); it != die.attributes().end()) {
            decl_line = it->get<int>();
        }

        std::string qualified_name = name;
        for (auto it = namespaces.rbegin(); it != namespaces.rend(); ++it) {
            if (!it->empty()) {
                qualified_name = *it + "::" + qualified_name;
            }
        }

        auto offset = die.offset();
        type_info_[offset] = qualified_name;

        if (!name.empty() && !decl_file.empty() && decl_line > 0) {
            // TODO: Handle class/struct/union/enum parsing here
            result_[decl_file][decl_line] = qualified_name;
        }
    }

    void parse_function(const dw::die &die, const namespace_list &namespaces)
    {
        std::string name;
        std::string decl_file;
        int decl_line = 0;

        if (auto it = die.find(dw::attribute_t::name); it != die.attributes().end()) {
            name = it->get<std::string>();
        }
        if (auto it = die.find(dw::attribute_t::decl_file); it != die.attributes().end()) {
            decl_file = posixpath::normpath(src_files_[it->get<int>()]);
        }
        if (auto it = die.find(dw::attribute_t::decl_line); it != die.attributes().end()) {
            decl_line = it->get<int>();
        }

        if (!name.empty() && !decl_file.empty() && decl_line > 0) {
            // TODO: Handle subprogram parsing here
            std::string qualified_name = name;
            for (auto it = namespaces.rbegin(); it != namespaces.rend(); ++it) {
                if (!it->empty()) {
                    qualified_name = *it + "::" + qualified_name;
                }
            }
            result_[decl_file][decl_line] = qualified_name;
        }
    }

    dw::compilation_unit &cu_;
    std::string name_;
    std::string base_dir_;
    std::vector<std::string> src_files_;
    std::unordered_map<std::size_t, std::string> type_info_{};
    std::unordered_map<std::string, std::map<int, std::string>> result_{};
};

class debug_parser {
public:
    explicit debug_parser(dw::debug &dbg) : dbg_(dbg) {}

    void parse()
    {
        for (auto &cu : dbg_) {
            cu_parser parser(cu);
            spdlog::info("parsing {}", parser.name());
            parser.parse();
            auto cu_base_dir = parser.base_dir();
            if (base_dir_.empty()) {
                base_dir_ = cu_base_dir;
            }
            base_dir_ = posixpath::commonpath({cu_base_dir, base_dir_});

            for (const auto &[filename, entry] : parser.result()) {
                result_[filename].insert(entry.begin(), entry.end());
            }
        }
    }

    [[nodiscard]] std::unordered_map<std::string, std::map<int, std::string>> result() const
    {
        return result_;
    }

    [[nodiscard]] std::string base_dir() const
    {
        return base_dir_;
    }

private:
    dw::debug &dbg_;
    std::string base_dir_;
    std::unordered_map<std::string, std::map<int, std::string>> result_{};
};
