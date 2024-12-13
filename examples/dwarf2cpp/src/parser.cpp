#include "dwarf2cpp/parser.h"

#include <spdlog/spdlog.h>

void debug_parser::parse()
{
    for (auto &cu : dbg_) {
        cu_parser parser(cu, *this);
        spdlog::info("parsing {}", parser.name());
        parser.parse();
        auto cu_base_dir = parser.base_dir();
        if (base_dir_.empty()) {
            base_dir_ = cu_base_dir;
        }
        base_dir_ = posixpath::commonpath({cu_base_dir, base_dir_});
        break;
    }
}

cu_parser::cu_parser(dw::compilation_unit &cu, debug_parser &dbg_parser) : cu_(cu), dbg_parser_(dbg_parser)
{
    auto &cu_die = cu.die();
    name_ = cu_die.find(dw::attribute_t::name)->get<std::string>();
    auto comp_dir = cu_die.find(dw::attribute_t::comp_dir)->get<std::string>();
    base_dir_ = posixpath::commonpath({name_, comp_dir});
    src_files_ = cu_die.src_files();
}
void cu_parser::parse()
{
    namespace_list namespaces;
    parse_children(cu_.die(), namespaces);
    // for (const auto &[offset, type] : type_info_) {
    //     spdlog::info("{} {}", offset, type);
    // }
}
void cu_parser::parse_children(const dw::die &die, namespace_list &namespaces) // NOLINT(*-no-recursion)
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

void cu_parser::parse_namespace(const dw::die &die, namespace_list &namespaces) // NOLINT(*-no-recursion)
{
    std::string name;
    if (auto it = die.find(dw::attribute_t::name); it != die.attributes().end()) {
        name = it->get<std::string>();
    }
    namespaces.push_back(name);
    parse_children(die, namespaces);
    namespaces.pop_back();
}

void cu_parser::parse_type(const dw::die &die, const namespace_list &namespaces) // NOLINT(*-no-recursion)
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
        if (die.tag() == dw::tag::base_type) {
            type_info_.emplace(die.offset(), name);
        }
        else {
            type_info_.emplace(die.offset(), get_qualified_name(namespaces, name));
        }
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
        case dw::tag::array_type: {
            int array_size = 0;
            for (const auto &child : die) {
                if (child.tag() != dw::tag::subrange_type) {
                    continue;
                }
                if (auto it = child.find(dw::attribute_t::count); it != child.attributes().end()) {
                    array_size += it->get<int>();
                    break;
                }
                if (auto it = child.find(dw::attribute_t::upper_bound); it != child.attributes().end()) {
                    array_size += it->get<int>() + 1;
                    break;
                }
            }
            type_name += "[" + std::to_string(array_size) + "]";
            break;
        }
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

    // result_[decl_file][decl_line] = get_qualified_name(namespaces, name);

    // TODO: parse types such as typedef, struct, class, union, enum here
}
void cu_parser::parse_function(const dw::die &die, const namespace_list &namespaces)
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

    auto func = std::make_unique<function>(namespaces);
    func->parse(die, *this);
    add_entry(decl_file, decl_line, std::move(func));
    // TODO: parse non-member function here
}

void cu_parser::add_entry(std::string file, std::size_t line, std::unique_ptr<entry> entry) const
{
    dbg_parser_.add_entry(std::move(file), line, std::move(entry));
}
