#include "dwarf2cpp/entry.h"

#include <sstream>

#include <llvm/Demangle/Demangle.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include "dwarf2cpp/algorithm.hpp"
#include "dwarf2cpp/parser.h"

namespace {
std::string to_string(dw::access a)
{
    switch (a) {
    case dw::access::public_:
        return "public";
    case dw::access::protected_:
        return "protected";
    case dw::access::private_:
        return "private";
    default:
        throw std::runtime_error("unknown access");
    }
}
} // namespace

std::string type_t::describe(const std::string &name) const
{
    std::stringstream ss;
    for (const auto &str : before_type) {
        ss << str << " ";
    }
    ss << type;
    for (const auto &str : after_type) {
        ss << " " << str;
    }
    if (!name.empty()) {
        ss << " " << name;
    }
    for (const auto &str : after_name) {
        ss << str;
    }
    return ss.str();
}

void parameter_t::parse(const dw::die &die, cu_parser &parser)
{
    if (die.attributes().contains(dw::attribute_t::name)) {
        name_ = die.attributes().at(dw::attribute_t::name)->get<std::string>();
    }
    if (die.attributes().contains(dw::attribute_t::type)) {
        const auto type = die.attributes().at(dw::attribute_t::type)->get<dw::die>();
        type_ = parser.get_type(type);
    }
    if (die.attributes().contains(dw::attribute_t::artificial)) {
        is_artificial_ = die.attributes().at(dw::attribute_t::artificial)->get<bool>();
    }
}

std::string parameter_t::to_source() const
{
    return type_.describe(name_);
}

void function_t::parse(const dw::die &die, cu_parser &parser)
{
    name_ = die.attributes().at(dw::attribute_t::name)->get<std::string>();
    if (die.attributes().contains(dw::attribute_t::linkage_name)) {
        linkage_name_ = die.attributes().at(dw::attribute_t::linkage_name)->get<std::string>();
        if (const char *demangled = llvm::itaniumDemangle(linkage_name_, true)) {
            std::string demangled_name = demangled;
            static const std::string const_str = "const";
            if (demangled_name.size() >= const_str.size()) {
                is_const_ = std::equal(const_str.rbegin(), const_str.rend(), demangled_name.rbegin());
            }
        }
    }
    if (die.attributes().contains(dw::attribute_t::type)) {
        const auto return_type = die.attributes().at(dw::attribute_t::type)->get<dw::die>();
        return_type_ = parser.get_type(return_type);
    }
    if (die.attributes().contains(dw::attribute_t::explicit_)) {
        is_explicit_ = die.attributes().at(dw::attribute_t::explicit_)->get<bool>();
    }
    if (die.attributes().contains(dw::attribute_t::virtuality)) {
        virtuality_ = static_cast<dw::virtuality>(die.attributes().at(dw::attribute_t::virtuality)->get<int>());
    }
    if (die.attributes().contains(dw::attribute_t::accessibility)) {
        access_ = static_cast<dw::access>(die.attributes().at(dw::attribute_t::accessibility)->get<int>());
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
    if (!linkage_name_.empty() && !starts_with(name_, "operator ")) {
        ss << return_type_.describe("");
    }
    if (is_explicit_) {
        ss << "explicit ";
    }
    ss << name_ << "(";
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
    if (die.attributes().contains(dw::attribute_t::name)) {
        name_ = die.attributes().at(dw::attribute_t::name)->get<std::string>();
    }
    if (die.attributes().contains(dw::attribute_t::type)) {
        type_ = parser.get_type(die.attributes().at(dw::attribute_t::type)->get<dw::die>());
    }
    if (die.attributes().contains(dw::attribute_t::data_member_location)) {
        member_location_ = die.attributes().at(dw::attribute_t::data_member_location)->get<std::size_t>();
    }
    if (die.attributes().contains(dw::attribute_t::accessibility)) {
        access_ = static_cast<dw::access>(die.attributes().at(dw::attribute_t::accessibility)->get<int>());
    }
    if (die.attributes().contains(dw::attribute_t::external)) {
        is_static = die.attributes().at(dw::attribute_t::external)->get<bool>();
    }
    if (die.attributes().contains(dw::attribute_t::const_value)) {
        default_value_ = die.attributes().at(dw::attribute_t::const_value)->get<std::int64_t>();
    }
    // TODO: handle anonymous struct here
}

std::string field_t::to_source() const
{
    std::stringstream ss;
    if (is_static) {
        ss << "static ";
    }
    ss << type_.describe(name_) << " ";
    if (default_value_.has_value()) {
        ss << " = ";
        if (type_.type == "float") {
            auto value = static_cast<std::int32_t>(default_value_.value());
            constexpr auto max_precision{std::numeric_limits<float>::digits10 + 1};
            ss << std::fixed << std::setprecision(max_precision) << *reinterpret_cast<float *>(&value);
        }
        else if (type_.type == "double") {
            auto value = default_value_.value();
            constexpr auto max_precision{std::numeric_limits<double>::digits10 + 1};
            ss << std::fixed << std::setprecision(max_precision) << *reinterpret_cast<double *>(&value);
        }
        else {
            ss << default_value_.value();
        }
    }
    ss << ";";
    if (member_location_.has_value()) {
        ss << " // +" << member_location_.value();
    }
    return ss.str();
}

void typedef_t::parse(const dw::die &die, cu_parser &parser)
{
    if (die.attributes().contains(dw::attribute_t::name)) {
        name_ = die.attributes().at(dw::attribute_t::name)->get<std::string>();
    }
    if (die.attributes().contains(dw::attribute_t::type)) {
        const auto type = die.attributes().at(dw::attribute_t::type)->get<dw::die>();
        type_ = parser.get_type(type);
    }
    if (die.attributes().contains(dw::attribute_t::accessibility)) {
        access_ = static_cast<dw::access>(die.attributes().at(dw::attribute_t::accessibility)->get<int>());
    }

    // TODO: handle anonymous struct here
}

std::string typedef_t::to_source() const
{
    return "using " + name_ + " = " + type_.describe("") + ";";
}

void enum_t::parse(const dw::die &die, cu_parser &parser)
{
    if (die.attributes().contains(dw::attribute_t::name)) {
        name_ = die.attributes().at(dw::attribute_t::name)->get<std::string>();
    }
    if (die.attributes().contains(dw::attribute_t::type)) {
        base_type_ = parser.get_type(die.attributes().at(dw::attribute_t::type)->get<dw::die>());
    }
    if (die.attributes().contains(dw::attribute_t::accessibility)) {
        access_ = static_cast<dw::access>(die.attributes().at(dw::attribute_t::accessibility)->get<int>());
    }
    for (const auto &child : die) {
        if (child.tag() != dw::tag::enumerator) {
            continue;
        }
        enumerator_t enumerator;
        enumerator.name = child.attributes().at(dw::attribute_t::name)->get<std::string>();
        enumerator.value = child.attributes().at(dw::attribute_t::const_value)->get<std::int64_t>();
        enumerators_.push_back(enumerator);
    }
}

std::string enum_t::to_source() const
{
    std::stringstream ss;
    ss << "enum class " << name_;
    if (!base_type_.has_value()) {
        ss << " : " << base_type_.value().describe("");
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
    if (die.attributes().contains(dw::attribute_t::name)) {
        name_ = die.attributes().at(dw::attribute_t::name)->get<std::string>();
    }
    if (die.attributes().contains(dw::attribute_t::byte_size)) {
        byte_size = die.attributes().at(dw::attribute_t::byte_size)->get<std::size_t>();
    }
    if (die.attributes().contains(dw::attribute_t::accessibility)) {
        access_ = static_cast<dw::access>(die.attributes().at(dw::attribute_t::accessibility)->get<int>());
    }

    for (const auto &child : die) {
        std::unique_ptr<entry> entry;
        std::size_t decl_line = 0;
        if (child.attributes().contains(dw::attribute_t::decl_line)) {
            decl_line = child.attributes().at(dw::attribute_t::decl_line)->get<std::size_t>();
        }

        switch (child.tag()) {
        case dw::tag::inheritance: {
            auto access = (is_class_ ? dw::access::private_ : dw::access::public_);
            if (child.attributes().contains(dw::attribute_t::accessibility)) {
                access = static_cast<dw::access>(child.attributes().at(dw::attribute_t::accessibility)->get<int>());
            }
            auto type = parser.get_type(child.attributes().at(dw::attribute_t::type)->get<dw::die>());
            base_classes_.emplace_back(access, type);
            break;
        }
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

        if (entry && decl_line > 0) {
            entry->parse(child, parser);
            members_[decl_line].emplace_back(std::move(entry));
        }
    }
}

std::string struct_t::to_source() const
{
    auto default_access = (is_class_ ? dw::access::private_ : dw::access::public_);

    std::stringstream ss;
    ss << (is_class_ ? "class " : "struct ") << name_;
    if (!base_classes_.empty()) {
        ss << ": ";
        for (auto i = 0; i < base_classes_.size(); ++i) {
            if (i > 0) {
                ss << ", ";
            }
            const auto &[access, base] = base_classes_[i];
            if (access != default_access) {
                ss << to_string(access) << " ";
            }
            ss << base.describe("");
        }
    }
    ss << " {\n";

    auto last_access = default_access;
    for (const auto &[decl_line, member] : members_) {
        for (auto &m : member) {
            auto current_access = m->access().value_or(default_access);
            if (current_access != last_access) {
                ss << to_string(current_access) << ":\n";
                last_access = current_access;
            }

#ifndef NDEBUG
            // ss << "// Line " << line << "\n";
#endif
            std::stringstream ss2(m->to_source());
            std::string line;
            while (std::getline(ss2, line)) {
                ss << "    " << line << "\n";
            }
        }
    }
    ss << "};\n";
    if (!name_.empty() && byte_size.has_value()) {
        ss << "static_assert(sizeof(" << name_ << ") == " << byte_size.value() << ");\n";
    }
    return ss.str();
}

void union_t::parse(const dw::die &die, cu_parser &parser)
{
    if (die.attributes().contains(dw::attribute_t::name)) {
        name_ = die.attributes().at(dw::attribute_t::name)->get<std::string>();
    }
    if (die.attributes().contains(dw::attribute_t::accessibility)) {
        access_ = static_cast<dw::access>(die.attributes().at(dw::attribute_t::accessibility)->get<int>());
    }

    for (const auto &child : die) {
        std::unique_ptr<entry> entry;
        std::size_t decl_line = 0;
        if (child.attributes().contains(dw::attribute_t::decl_line)) {
            decl_line = child.attributes().at(dw::attribute_t::decl_line)->get<std::size_t>();
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
