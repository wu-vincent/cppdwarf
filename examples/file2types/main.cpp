#include <filesystem>
#include <fstream>
#include <unordered_set>

#include <argparse/argparse.hpp>
#include <cppdwarf/cppdwarf.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace dw = cppdwarf;
namespace fs = std::filesystem;

using file_entry_storage = std::unordered_map<std::string, std::unordered_set<std::string>>;

nlohmann::json result;

std::string join(const std::vector<std::string> &strings, const std::string &delimiter)
{
    std::string result;
    for (size_t i = 0; i < strings.size(); ++i) {
        result += strings[i];
        if (i < strings.size() - 1) {
            result += delimiter;
        }
    }
    return result;
}

void parse(const dw::die &die, std::vector<std::string> &parents, const std::vector<std::string> &src_files)
{
    for (const auto &child : die) {
        const auto tag = child.tag();
        std::string name;
        if (auto it = child.find(dw::attribute_t::name); it != child.attributes().end()) {
            name = it->get<std::string>();
        }

        switch (tag) {
        case dw::tag::namespace_: {
            if (name.empty()) {
                name = "__anonymous_namespace__";
            }
            parents.push_back(name);
            parse(child, parents, src_files);
            parents.pop_back();
            break;
        }
        case dw::tag::class_type:
        case dw::tag::structure_type:
        case dw::tag::union_type:
        case dw::tag::enumeration_type: {
            std::string decl_file;
            int decl_line = 0;
            if (auto it = child.find(dw::attribute_t::decl_file); it != child.attributes().end()) {
                decl_file = src_files.at(it->get<int>());
            }
            if (auto it = child.find(dw::attribute_t::decl_line); it != child.attributes().end()) {
                decl_line = it->get<int>();
            }

            if (!name.empty() && !decl_file.empty() && decl_line > 0) {
                parents.push_back(name);
                result[decl_file].push_back({
                    {"line", decl_line},
                    {"name", join(parents, "::")},
                });
                parse(child, parents, src_files);
                parents.pop_back();
            }
            break;
        }
        default: {
            break;
        }
        }
    }
}

int main(int argc, char *argv[])
{
    argparse::ArgumentParser parser("cpp2dwarf");
    parser.add_argument("path").help("path to a DWARF debug symbol file");
    try {
        parser.parse_args(argc, argv);
    }
    catch (std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    auto path = parser.get<std::string>("path");
    auto debug = dw::debug(path);
    for (const auto &cu : debug) {
        auto &cu_die = cu.die();
        std::string die_name = cu_die.find(dw::attribute_t::name)->get<std::string>();
        spdlog::info("{}", die_name);
        std::vector<std::string> parents;
        parse(cu_die, parents, cu_die.src_files());
    }

    std::ofstream output_file("output.json");
    if (output_file.is_open()) {
        output_file << result.dump(4); // Pretty print with indentation of 4 spaces
        output_file.close();
        spdlog::info("JSON result successfully written to 'output.json'");
    }
    else {
        spdlog::error("Failed to open file for writing JSON result");
    }

    return 0;
}
