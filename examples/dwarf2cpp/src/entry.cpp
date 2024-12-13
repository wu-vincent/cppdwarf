#include "dwarf2cpp/entry.h"

#include <sstream>

#include <llvm/Demangle/Demangle.h>

#include "dwarf2cpp/parser.h"

void parameter_t::parse(const dw::die &die, cu_parser &parser)
{
    if (const auto it = die.attributes().find(dw::attribute_t::name); it != die.attributes().end()) {
        name_ = it->get<std::string>();
    }
    if (const auto it = die.attributes().find(dw::attribute_t::type); it != die.attributes().end()) {
        const auto type = it->get<dw::die>();
        type_ = parser.get_type(it->get<dw::die>());
    }
}

std::string parameter_t::to_source() const
{
    return type_ + " " + name_;
}

void function_t::parse(const dw::die &die, cu_parser &parser)
{
    name_ = die.attributes().at(dw::attribute_t::name).get<std::string>();
    if (const auto it = die.attributes().find(dw::attribute_t::linkage_name); it != die.attributes().end()) {
        linkage_name_ = it->get<std::string>();
        if (const char *demangled = llvm::itaniumDemangle(linkage_name_, true)) {
            std::string demangled_name = demangled;
            static const std::string const_str = "const";
            if (demangled_name.size() >= const_str.size()) {
                is_const_ = std::equal(const_str.rbegin(), const_str.rend(), demangled_name.rbegin());
            }
        }
    }
    if (const auto it = die.attributes().find(dw::attribute_t::type); it != die.attributes().end()) {
        const auto return_type = it->get<dw::die>();
        return_type_ = parser.get_type(it->get<dw::die>());
    }
    for (const auto &child : die) {
        switch (child.tag()) {
        case dw::tag::formal_parameter: {
            auto param = std::make_unique<parameter_t>();
            param->parse(child, parser);
            parameters_.push_back(std::move(param));
        }
        default:
            // TODO: extract information about template params
            break;
        }
    }
}

std::string function_t::to_source() const
{
    std::stringstream ss;
    // if (!linkage_name_.empty()) {
    //     ss << "// " << linkage_name_ << "\n";
    // }
    ss << return_type_ << " " << name_;
    ss << "(";
    for (auto i = 0; i < parameters_.size(); ++i) {
        if (i > 0) {
            ss << ", ";
        }
        ss << parameters_[i]->to_source();
    }
    ss << ")";
    if (is_const_) {
        ss << " const";
    }
    ss << ";";
    return ss.str();
}

void field_t::parse(const dw::die &die, cu_parser &parser)
{
    if (const auto it = die.attributes().find(dw::attribute_t::name); it != die.attributes().end()) {
        name_ = it->get<std::string>();
    }
    if (const auto it = die.attributes().find(dw::attribute_t::type); it != die.attributes().end()) {
        type_ = parser.get_type(it->get<dw::die>());
    }
    // TODO: handle anonymous struct here
}

std::string field_t::to_source() const
{
    return type_ + " " + name_ + ";";
}

void typedef_t::parse(const dw::die &die, cu_parser &parser)
{
    if (const auto it = die.attributes().find(dw::attribute_t::name); it != die.attributes().end()) {
        name_ = it->get<std::string>();
    }
    if (const auto it = die.attributes().find(dw::attribute_t::type); it != die.attributes().end()) {
        const auto type = it->get<dw::die>();
        type_ = parser.get_type(it->get<dw::die>());
    }
    // TODO: handle anonymous struct here
}

std::string typedef_t::to_source() const
{
    return "using " + name_ + " = " + type_ + ";";
}

void enum_t::parse(const dw::die &die, cu_parser &parser)
{
    if (const auto it = die.attributes().find(dw::attribute_t::name); it != die.attributes().end()) {
        name_ = it->get<std::string>();
    }
    if (const auto it = die.attributes().find(dw::attribute_t::type); it != die.attributes().end()) {
        base_type_ = parser.get_type(it->get<dw::die>());
    }
    for (const auto &child : die) {
        if (child.tag() != dw::tag::enumerator) {
            continue;
        }
        enumerator_t enumerator;
        enumerator.name = child.attributes().at(dw::attribute_t::name).get<std::string>();
        enumerator.value = child.attributes().at(dw::attribute_t::const_value).get<std::int64_t>();
        enumerators_.push_back(enumerator);
    }
}

std::string enum_t::to_source() const
{
    std::stringstream ss;
    ss << "enum class " << name_;
    if (!base_type_.empty()) {
        ss << " : " << base_type_;
    }
    ss << " {\n";
    for (const auto &[name, value] : enumerators_) {
        ss << "    " << name << " = " << value << ",\n";
    }
    ss << "};";
    return ss.str();
}

void struct_t::parse(const dw::die &die, cu_parser &parser)
{
    if (const auto it = die.attributes().find(dw::attribute_t::name); it != die.attributes().end()) {
        name_ = it->get<std::string>();
    }

    auto ns = namespaces();
    if (!name_.empty()) {
        ns.push_back(name_);
    }

    for (const auto &child : die) {
        std::unique_ptr<entry> entry;
        std::size_t decl_line = 0;
        if (const auto it = child.attributes().find(dw::attribute_t::decl_line); it != child.attributes().end()) {
            decl_line = it->get<std::size_t>();
        }

        switch (child.tag()) {
        case dw::tag::inheritance:
            // TODO: parse inheritance
            break;
        case dw::tag::member:
            entry = std::make_unique<field_t>();
            break;
        case dw::tag::subprogram:
            entry = std::make_unique<function_t>();
            break;
        case dw::tag::union_type:
            entry = std::make_unique<union_t>();
            break;
        case dw::tag::structure_type:
            entry = std::make_unique<struct_t>(false);
            break;
        case dw::tag::class_type:
            entry = std::make_unique<struct_t>(true);
            break;
        case dw::tag::enumeration_type:
            entry = std::make_unique<enum_t>();
            break;
        case dw::tag::typedef_:
            entry = std::make_unique<typedef_t>();
            break;
        default:
            break;
        }
        if (entry) {
            entry->parse(child, parser);
            members_[decl_line] = std::move(entry);
        }
    }
}

std::string struct_t::to_source() const
{
    std::stringstream ss;
    ss << (is_class_ ? "class " : "struct ") << name_ << " {\n";
    for (const auto &[line, member] : members_) {
#ifndef NDEBUG
        // ss << "// Line " << line << "\n";
#endif
        ss << member->to_source() << "\n";
    }
    ss << "};\n";
    return ss.str();
}

void union_t::parse(const dw::die &die, cu_parser &parser)
{
    if (const auto it = die.attributes().find(dw::attribute_t::name); it != die.attributes().end()) {
        name_ = it->get<std::string>();
    }

    for (const auto &child : die) {
        std::unique_ptr<entry> entry;
        std::size_t decl_line = 0;
        if (const auto it = child.attributes().find(dw::attribute_t::decl_line); it != child.attributes().end()) {
            decl_line = it->get<std::size_t>();
        }

        switch (child.tag()) {
        case dw::tag::member:
            entry = std::make_unique<field_t>();
            break;
        case dw::tag::subprogram:
            entry = std::make_unique<function_t>();
            break;
        case dw::tag::union_type:
            entry = std::make_unique<union_t>();
            break;
        case dw::tag::structure_type:
            entry = std::make_unique<struct_t>(false);
            break;
        case dw::tag::class_type:
            entry = std::make_unique<struct_t>(true);
            break;
        case dw::tag::enumeration_type:
            entry = std::make_unique<enum_t>();
            break;
        case dw::tag::typedef_:
            entry = std::make_unique<typedef_t>();
            break;
        default:
            break;
        }
        if (entry) {
            entry->parse(child, parser);
            members_[decl_line] = std::move(entry);
        }
    }
}

std::string union_t::to_source() const
{
    std::stringstream ss;
    ss << "union" << name_ << " {\n";
    for (const auto &[line, member] : members_) {
        // ss << "// Line " << line << "\n";
        ss << member->to_source() << "\n";
    }
    ss << "};\n";
    return ss.str();
}
