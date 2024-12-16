#include "dwarf2cpp/parser.h"

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

const debug_parser::result &debug_parser::parse()
{
    int i = 0;
    for (auto &cu : dbg_) {
        auto &cu_die = cu.die();
        auto name = cu_die.find(dw::attribute_t::name)->get<std::string>();
        auto comp_dir = cu_die.find(dw::attribute_t::comp_dir)->get<std::string>();
        auto base_dir = posixpath::commonpath({name, comp_dir});

        spdlog::info("[{:<4}] parsing {}", ++i, name);
        cu_parser parser(cu, *this);
        parser.parse();
        if (result_.base_dir.empty()) {
            result_.base_dir = base_dir;
        }
        result_.base_dir = posixpath::commonpath({base_dir, result_.base_dir});
    }
    return result_;
}

cu_parser::cu_parser(dw::compilation_unit &cu, debug_parser &dbg_parser) : cu_(cu), dbg_parser_(dbg_parser)
{
    auto &cu_die = cu.die();
    src_files_ = cu_die.src_files();
}

void cu_parser::parse()
{
    std::vector<std::string> parents;

    // first pass: parse all types with names
    parse_types(cu_.die(), parents);

    // second pass: parse all functions and classes
    parse_children(cu_.die(), parents);
}

type_t cu_parser::get_type(const dw::die &die)
{
    auto it = known_types_.find(die.offset());
    if (it == known_types_.end()) {
        std::unique_ptr<dw::die> type;
        type_t new_type;
        if (auto attr = die.find(dw::attribute_t::type); attr != die.attributes().end()) {
            type = attr->get<std::unique_ptr<dw::die>>();
            new_type = get_type(*type);
        }

        switch (die.tag()) {
        case dw::tag::array_type: {
            int array_size = 0;
            for (const auto &child : die) {
                if (child.tag() != dw::tag::subrange_type) {
                    continue;
                }
                if (auto attr = child.find(dw::attribute_t::count); attr != child.attributes().end()) {
                    array_size += attr->get<int>();
                    break;
                }
                if (auto attr = child.find(dw::attribute_t::upper_bound); attr != child.attributes().end()) {
                    array_size += attr->get<int>() + 1;
                    break;
                }
            }
            new_type.suffixes.emplace_back("[" + std::to_string(array_size) + "]");
            break;
        }
        case dw::tag::pointer_type: {
            new_type.prefixes.emplace_back("*");
            break;
        }
        case dw::tag::reference_type: {
            new_type.prefixes.emplace_back("&");
            break;
        }
        case dw::tag::rvalue_reference_type: {
            new_type.prefixes.emplace_back("&&");
            break;
        }
        case dw::tag::const_type: {
            new_type.prefixes.emplace_back("const");
            break;
        }
        case dw::tag::atomic_type: {
            new_type.prefixes.insert(new_type.prefixes.begin(), "_Atomic");
            break;
        }
        case dw::tag::restrict_type: {
            new_type.prefixes.insert(new_type.prefixes.begin(), "restrict");
            break;
        }
        case dw::tag::ptr_to_member_type: {
            auto containing_type = die.attributes().at(dw::attribute_t::containing_type).get<dw::die>();
            new_type.prefixes.emplace_back(" " + get_type(containing_type).describe("") + "::" + "*");
            break;
        }
        case dw::tag::subroutine_type: {
            break;
        }
        default: {
            std::stringstream ss;
            ss << "<" << die.tag() << ">";
            new_type.prefixes.emplace_back(ss.str());
            break;
        }
        }
        known_types_[die.offset()] = new_type;
    }
    return known_types_.at(die.offset());
}

void cu_parser::parse_types(const dw::die &die, namespace_list &parents) // NOLINT(*-no-recursion)
{
    for (const auto &child : die) {
        if (known_types_.find(child.offset()) != known_types_.end()) {
            continue;
        }

        const auto tag = child.tag();
        std::string name;
        if (auto it = child.find(dw::attribute_t::name); it != child.attributes().end()) {
            name = it->get<std::string>();
        }

        switch (tag) {
        case dw::tag::namespace_: {
            if (name == "__1" && parents.size() == 1 && parents.at(0) == "std") {
                parse_types(child, parents);
            }
            else {
                parents.push_back(name);
                parse_types(child, parents);
                parents.pop_back();
            }
            break;
        }
        case dw::tag::base_type: {
            if (name.empty()) {
                name = "void";
            }
            known_types_[child.offset()] = {{get_qualified_name(parents, name)}};
            break;
        }
        case dw::tag::typedef_: {
            if (name.empty()) {
                throw std::runtime_error("invalid typedef");
            }
            known_types_[child.offset()] = {{get_qualified_name(parents, name)}};
            break;
        }
        case dw::tag::unspecified_type: {
            if (name.empty()) {
                throw std::runtime_error("invalid unspecified type");
            }
            known_types_[child.offset()] = {{get_qualified_name(parents, name)}};
            break;
        }
        case dw::tag::class_type:
        case dw::tag::structure_type:
        case dw::tag::union_type:
        case dw::tag::enumeration_type: {
            if (!name.empty()) {
                known_types_[child.offset()] = {{get_qualified_name(parents, name)}};
            }
            else {
                known_types_[child.offset()] = {{"<unnamed>"}};
            }
            parents.push_back(name);
            parse_types(child, parents);
            parents.pop_back();
            break;
        }
        default: {
            break;
        }
        }
    }
}

void cu_parser::parse_children(const dw::die &die, namespace_list &namespaces) // NOLINT(*-no-recursion)
{
    for (const auto &child : die) {
        const auto tag = child.tag();

        std::string name;
        if (auto it = child.find(dw::attribute_t::name); it != child.attributes().end()) {
            name = it->get<std::string>();
        }

        if (tag == dw::tag::namespace_) {
            if (name == "__1" && namespaces.size() == 1 && namespaces.at(0) == "std") {
                parse_children(child, namespaces);
            }
            else {
                namespaces.push_back(name);
                parse_children(child, namespaces);
                namespaces.pop_back();
            }
            continue;
        }

        std::string decl_file;
        int decl_line = 0;
        if (auto it = child.find(dw::attribute_t::decl_file); it != child.attributes().end()) {
            int file_index = it->get<int>();
            if (cu_.version() < 5) {
                file_index -= 1;
            }
            decl_file = src_files_.at(file_index);
        }
        if (auto it = child.find(dw::attribute_t::decl_line); it != child.attributes().end()) {
            decl_line = it->get<int>();
        }

        if (name.empty() || decl_file.empty() || decl_line <= 0) {
            continue;
        }
        std::unique_ptr<entry> entry;
        switch (tag) {
        case dw::tag::class_type: {
            entry = std::make_unique<struct_t>(true, namespaces);
            break;
        }
        case dw::tag::structure_type: {
            entry = std::make_unique<struct_t>(false, namespaces);
            break;
        }
        case dw::tag::union_type: {
            entry = std::make_unique<union_t>(namespaces);
            break;
        }
        case dw::tag::enumeration_type: {
            entry = std::make_unique<enum_t>(namespaces);
            break;
        }
        case dw::tag::subprogram: {
            entry = std::make_unique<function_t>(false, namespaces);
            break;
        }
        case dw::tag::typedef_: {
            entry = std::make_unique<typedef_t>(namespaces);
            break;
        }
        default:
            break;
        }

        if (entry) {
            entry->parse(child, *this);
            add_entry(decl_file, decl_line, std::move(entry));
        }
    }
}

void cu_parser::add_entry(std::string file, std::size_t line, std::unique_ptr<entry> entry) const
{
    dbg_parser_.add_entry(std::move(file), line, std::move(entry));
}
