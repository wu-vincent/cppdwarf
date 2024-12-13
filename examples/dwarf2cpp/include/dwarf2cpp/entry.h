#pragma once

#include <unordered_map>
#include <utility>

#include <cppdwarf/cppdwarf.hpp>

namespace dw = cppdwarf;

class cu_parser;

class entry {
public:
    using type_info = std::unordered_map<std::size_t, std::string>;
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

class parameter : public entry {
public:
    using entry::entry;
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;

private:
    std::string name_;
    std::string type_;
};

class function : public entry {
public:
    using entry::entry;
    void parse(const dw::die &die, cu_parser &parser) override;
    [[nodiscard]] std::string to_source() const override;

private:
    std::string name_;
    std::string linkage_name_;
    std::string return_type_{"void"};
    std::vector<std::unique_ptr<parameter>> parameters_;
    bool is_const_{false};
};
