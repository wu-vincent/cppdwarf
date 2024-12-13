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
    if (const auto it = die.attributes().find(dw::attribute_t::artificial); it != die.attributes().end()) {
        is_artificial_ = it->get<bool>();
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
    if (const auto it = die.attributes().find(dw::attribute_t::virtuality); it != die.attributes().end()) {
        virtuality_ = static_cast<dw::virtuality>(it->get<int>());
    }

    int param_index = 0;
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
        param_index++;
    }
}

std::string function_t::to_source() const
{
    std::stringstream ss;
    // if (!linkage_name_.empty()) {
    //     ss << "// " << linkage_name_ << "\n";
    // }
    auto it = parameters_.begin();
    if (is_member_ && it != parameters_.end()) {
        auto &first_param = *it;
        if (first_param->is_artificial()) {
            it = std::next(it);
        }
        else {
            ss << "static ";
        }
    }
    if (virtuality_ > dw::virtuality::none) {
        ss << "virtual ";
    }
    ss << return_type_ << " " << name_;
    ss << "(";
    for (; it != parameters_.end(); ++it) {
        ss << (*it)->to_source();
        if (it < parameters_.end() - 1) {
            ss << ", ";
        }
    }
    ss << ")";
    if (is_const_) {
        ss << " const";
    }
    if (virtuality_ == dw::virtuality::pure_virtual) {
        ss << " = 0";
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
    if (const auto it = die.attributes().find(dw::attribute_t::data_member_location); it != die.attributes().end()) {
        member_location_ = it->get<std::size_t>();
    }
    // TODO: handle anonymous struct here
}

std::string field_t::to_source() const
{
    std::stringstream ss;
    ss << type_ << " " << name_ << ";";
    if (member_location_.has_value()) {
        ss << " // +" << member_location_.value();
    }
    return ss.str();
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

    if (const auto it = die.attributes().find(dw::attribute_t::byte_size); it != die.attributes().end()) {
        byte_size = it->get<std::size_t>();
    }

    for (const auto &child : die) {
        std::unique_ptr<entry> entry;
        std::size_t decl_line = 0;
        if (const auto it = child.attributes().find(dw::attribute_t::decl_line); it != child.attributes().end()) {
            decl_line = it->get<std::size_t>();
        }

        if (decl_line <= 0) {
            continue;
        }

        switch (child.tag()) {
        case dw::tag::inheritance:
            // TODO: parse inheritance
            break;
        case dw::tag::member:
            entry = std::make_unique<field_t>();
            break;
        case dw::tag::subprogram:
            entry = std::make_unique<function_t>(true);
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
    for (const auto &[decl_line, member] : members_) {
#ifndef NDEBUG
        // ss << "// Line " << line << "\n";
#endif
        std::stringstream ss2(member->to_source());
        std::string line;
        while (std::getline(ss2, line)) {
            ss << "    " << line << "\n";
        }
    }
    ss << "};\n";
    if (byte_size.has_value()) {
        ss << "static_assert(sizeof(" << name_ << ") == " << byte_size.value() << ");\n";
    }
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
            entry = std::make_unique<function_t>(true);
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
    for (const auto &[decl_line, member] : members_) {
        // ss << "// Line " << line << "\n";
        std::stringstream ss2(member->to_source());
        std::string line;
        while (std::getline(ss2, line)) {
            ss << "    " << line << "\n";
        }
    }
    ss << "};\n";
    return ss.str();
}
