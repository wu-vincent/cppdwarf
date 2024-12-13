#pragma once

#include <map>
#include <unordered_map>
#include <utility>

#include <cppdwarf/cppdwarf.hpp>

namespace dw = cppdwarf;

class cu_parser;

class entry {
public:
    using namespace_list = std::vector<std::string>;

    explicit entry(namespace_list namespaces = {}) : namespaces_(std::move(namespaces)){};

    virtual ~entry() = default;
    virtual void parse(const dw::die &die, cu_parser &parser) = 0;
    [[nodiscard]] virtual std::string to_source() const = 0;

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
    std::string type_{"<check>"};
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

private:
    std::string name_;
    std::string linkage_name_;
    std::string return_type_{"void"};
    std::vector<std::unique_ptr<parameter_t>> parameters_;
    bool is_const_{false};
    bool is_member_{false};
};

class field_t : public entry {
public:
    using entry::entry;
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;

private:
    std::string name_;
    std::string type_{"<check>"};
};

class typedef_t : public entry {
public:
    using entry::entry;
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;

private:
    std::string name_;
    std::string type_{"<check>"};
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

private:
    std::string name_;
    std::string base_type_;
    std::vector<enumerator_t> enumerators_;
};

class struct_t : public entry {
public:
    explicit struct_t(bool is_class, namespace_list namespaces = {}) : entry(std::move(namespaces)), is_class_(is_class)
    {
    }
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;

private:
    std::string name_;
    bool is_class_;
    std::map<std::size_t, std::unique_ptr<entry>> members_;
};

class union_t : public entry {
public:
    using entry::entry;
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;

private:
    std::string name_;
    std::map<std::size_t, std::unique_ptr<entry>> members_;
};
