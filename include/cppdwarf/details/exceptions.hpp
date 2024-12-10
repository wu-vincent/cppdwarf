#pragma once

#include <stdexcept>
#include <string>

namespace cppdwarf {

class exception : public std::exception {
public:
    [[nodiscard]] const char *what() const noexcept override
    {
        return m.what();
    }

protected:
    explicit exception(const std::string &message) : m(message) {} // NOLINT(bugprone-throw-keyword-missing)
    explicit exception(const char *message) : m(message) {}        // NOLINT(bugprone-throw-keyword-missing)

private:
    std::runtime_error m;
};

class init_error : public exception {
public:
    explicit init_error(const std::string &message) : exception(message) {}
    explicit init_error(const char *message) : exception(message) {}
};

} // namespace cppdwarf
