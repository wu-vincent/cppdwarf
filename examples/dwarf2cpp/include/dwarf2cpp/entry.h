#pragma once

#include <map>
#include <optional>
#include <unordered_map>
#include <utility>

#include <cppdwarf/cppdwarf.hpp>

namespace dw = cppdwarf;

class cu_parser;

struct type_t {
    std::string type = "void";
    std::vector<std::string> before_type;
    std::vector<std::string> after_type;
    std::vector<std::string> after_name;
    [[nodiscard]] std::string describe(const std::string &name) const;
};

class entry {
public:
    using namespace_list = std::vector<std::string>;

    explicit entry(namespace_list namespaces = {}) : namespaces_(std::move(namespaces)){};

    virtual ~entry() = default;
    virtual void parse(const dw::die &die, cu_parser &parser) = 0;
    [[nodiscard]] virtual std::string to_source() const = 0;
    [[nodiscard]] virtual std::optional<dw::access> access() const
    {
        return std::nullopt;
    }

    [[nodiscard]] namespace_list namespaces() const
    {
        return namespaces_;
    }

private:
    namespace_list namespaces_;
};

class parameter_t : public entry {
public:
    using entry::entry;
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;

    [[nodiscard]] bool is_artificial() const
    {
        return is_artificial_;
    }

private:
    std::string name_;
    type_t type_;
    bool is_artificial_{false};
};

class function_t : public entry {
public:
    explicit function_t(bool is_member, namespace_list namespaces = {})
        : entry(std::move(namespaces)), is_member_(is_member)
    {
    }
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;
    [[nodiscard]] std::optional<dw::access> access() const override
    {
        return access_;
    }

private:
    std::string name_;
    std::string linkage_name_;
    type_t return_type_;
    std::vector<std::unique_ptr<parameter_t>> parameters_;
    bool is_const_{false};
    bool is_member_{false};
    bool is_explicit_{false};
    dw::virtuality virtuality_{dw::virtuality::none};
    std::optional<dw::access> access_;
};

class field_t : public entry {
public:
    using entry::entry;
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;
    [[nodiscard]] std::optional<dw::access> access() const override
    {
        return access_;
    }

private:
    std::string name_;
    type_t type_;
    std::optional<std::size_t> member_location_;
    std::optional<dw::access> access_;
    bool is_static{false};
    std::optional<std::int64_t> default_value_;
};

class typedef_t : public entry {
public:
    using entry::entry;
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;
    [[nodiscard]] std::optional<dw::access> access() const override
    {
        return access_;
    }

private:
    std::string name_;
    type_t type_;
    std::optional<dw::access> access_;
};

class enum_t : public entry {
public:
    struct enumerator_t {
        std::string name;
        std::int64_t value;
    };
    using entry::entry;
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;
    [[nodiscard]] std::optional<dw::access> access() const override
    {
        return access_;
    }

private:
    std::string name_;
    std::optional<type_t> base_type_;
    std::vector<enumerator_t> enumerators_;
    std::optional<dw::access> access_;
};

class struct_t : public entry {
public:
    explicit struct_t(bool is_class, namespace_list namespaces = {}) : entry(std::move(namespaces)), is_class_(is_class)
    {
    }
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;
    [[nodiscard]] std::optional<dw::access> access() const override
    {
        return access_;
    }

private:
    std::string name_;
    bool is_class_;
    std::map<std::size_t, std::vector<std::unique_ptr<entry>>> members_;
    std::optional<std::size_t> byte_size;
    std::vector<std::pair<dw::access, type_t>> base_classes_;
    std::optional<dw::access> access_;
};

class union_t : public entry {
public:
    using entry::entry;
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;
    [[nodiscard]] std::optional<dw::access> access() const override
    {
        return access_;
    }

private:
    std::string name_;
    std::map<std::size_t, std::unique_ptr<entry>> members_;
    std::optional<dw::access> access_;
};
