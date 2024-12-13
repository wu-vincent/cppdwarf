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
        parser.parse_type(type, namespaces());
        type_ = parser.get_type(type.offset());
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
        parser.parse_type(return_type, namespaces());
        return_type_ = parser.get_type(return_type.offset());
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
    if (!linkage_name_.empty()) {
        ss << "// " << linkage_name_ << "\n";
    }
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

void typedef_t::parse(const dw::die &die, cu_parser &parser)
{
    if (const auto it = die.attributes().find(dw::attribute_t::name); it != die.attributes().end()) {
        name_ = it->get<std::string>();
    }
    if (const auto it = die.attributes().find(dw::attribute_t::type); it != die.attributes().end()) {
        const auto type = it->get<dw::die>();
        parser.parse_type(type, namespaces());
        type_ = parser.get_type(type.offset());
    }
    // TODO: handle anonymous struct here
}

std::string typedef_t::to_source() const
{
    return "using " + name_ + " = " + type_ + ";";
}
