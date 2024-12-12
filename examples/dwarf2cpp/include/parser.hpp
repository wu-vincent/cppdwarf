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
        parse_children(cu_.die(), namespaces);

        for (const auto &[offset, type] : type_info_) {
            spdlog::info("{} {}", offset, type);
        }
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
    void parse_children(const dw::die &die, namespace_list &namespaces) // NOLINT(*-no-recursion)
    {
        for (const auto &child : die) {
            const auto tag = child.tag();
            switch (tag) {
            case dw::tag::namespace_:
                // A namespace can contain declarations from multiple files - we don't want this
                // To solve this, we go inside the namespaces and start there again as the top level
                parse_namespace(child, namespaces);
                break;
            case dw::tag::base_type:
            case dw::tag::unspecified_type:
            case dw::tag::typedef_:
            case dw::tag::class_type:
            case dw::tag::structure_type:
            case dw::tag::union_type:
            case dw::tag::enumeration_type:
            case dw::tag::array_type:
            case dw::tag::pointer_type:
            case dw::tag::const_type:
            case dw::tag::rvalue_reference_type:
            case dw::tag::reference_type:
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
        parse_children(die, namespaces);
        namespaces.pop_back();
    }

    // parse a non-member type, for member types, see parse_member_type
    void parse_type(const dw::die &die, const namespace_list &namespaces)
    {
        if (type_info_.find(die.offset()) != type_info_.end()) {
            // we've already fully parsed the type
            return;
        }
        std::string name;
        std::string decl_file;
        int decl_line = 0;
        std::unique_ptr<dw::die> type;
        if (auto it = die.find(dw::attribute_t::name); it != die.attributes().end()) {
            name = it->get<std::string>();
        }
        if (auto it = die.find(dw::attribute_t::decl_file); it != die.attributes().end()) {
            decl_file = posixpath::normpath(src_files_[it->get<int>()]);
        }
        if (auto it = die.find(dw::attribute_t::decl_line); it != die.attributes().end()) {
            decl_line = it->get<int>();
        }
        if (auto it = die.find(dw::attribute_t::type); it != die.attributes().end()) {
            type = it->get<std::unique_ptr<dw::die>>();
        }

        if (!name.empty()) { // we already know the name, no need to resolve the type chain
            type_info_.emplace(die.offset(), get_qualified_name(namespaces, name));
        }
        else if (die.tag() == dw::tag::base_type) {
            // base_type without a name implies void
            type_info_.emplace(die.offset(), "void");
        }
        else if (type != nullptr) {
            // we don't have a name and we are not a base_type (i.e. not 'void'), but we have a ref to type.
            // solve the type chain iteratively.
            parse_type(*type, namespaces);
            // at this point we should know everything about the underlying type
            auto type_name = type_info_.at(type->offset());
            switch (die.tag()) {
            case dw::tag::const_type:
                type_name += " const";
                break;
            case dw::tag::pointer_type:
                type_name += "*";
                break;
            case dw::tag::array_type:
                type_name += "[]";
                break;
            case dw::tag::reference_type:
                type_name += "&";
                break;
            case dw::tag::rvalue_reference_type:
                type_name += "&&";
                break;
            case dw::tag::volatile_type:
                type_name = "volatile " + type_name;
                break;
            default:
                break;
            }
            type_info_.emplace(die.offset(), type_name);
        }
        else {
            // no name, no type
            std::stringstream ss;
            ss << '<' << die.tag() << '>';
            std::string type_name = ss.str();
            switch (die.tag()) {
            case dw::tag::const_type:
                type_name = "void const";
                break;
            case dw::tag::pointer_type:
                type_name = "void *";
                break;
            case dw::tag::reference_type:
                type_name = "void &";
                break;
            default:
                break;
            }
            type_info_.emplace(die.offset(), type_name);
        }

        if (name.empty() || decl_file.empty() || decl_line <= 0) {
            return;
        }

        result_[decl_file][decl_line] = get_qualified_name(namespaces, name);

        // TODO: parse types such as typedef, struct, class, union, enum here
    }

    // parse a non-member function, for member functions, see parse_member_function
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

        if (name.empty() || decl_file.empty() || decl_line <= 0) {
            return;
        }

        result_[decl_file][decl_line] = get_qualified_name(namespaces, name);
        // TODO: parse non-member function here
    }

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
            break;
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
