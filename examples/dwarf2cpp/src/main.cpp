#include <filesystem>
#include <unordered_set>

#include <argparse/argparse.hpp>
#include <cppdwarf/cppdwarf.hpp>
#include <spdlog/fmt/std.h>
#include <spdlog/spdlog.h>

#include "dwarf2cpp/parser.h"
#include "dwarf2cpp/posixpath.hpp"

namespace dw = cppdwarf;
namespace fs = std::filesystem;

using file_entry_storage = std::unordered_map<std::string, std::unordered_set<std::string>>;

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
    auto dbg_parser = debug_parser(debug);
    auto &result = dbg_parser.parse();
    auto base_dir = result.base_dir;
    for (const auto &[filename, content] : result.files) {
        if (posixpath::commonpath({filename, base_dir}) != base_dir) {
            continue;
        }
        auto relpath = posixpath::relpath(filename, base_dir);

        fs::path output_file = fs::path("output") / relpath;
        create_directories(output_file.parent_path());

        spdlog::info("writing to {}", output_file);
        std::ofstream out(output_file.string());
        out << content;
    }
    return 0;
}
