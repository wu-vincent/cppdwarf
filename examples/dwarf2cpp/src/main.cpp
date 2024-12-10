#include <unordered_set>

#include <argparse/argparse.hpp>
#include <cppdwarf/cppdwarf.hpp>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include "posixpath.hpp"

namespace dw = cppdwarf;

template <>
struct fmt::formatter<dw::die> : ostream_formatter {};

using file_entry_storage = std::unordered_map<std::string, std::unordered_set<std::string>>;

void parse_children(const dw::die &die, file_entry_storage &storage, const std::vector<std::string> &src_files)
{
    for (const auto &child : die) {
        switch (child.tag()) {
        case dw::tag::namespace_: {
            parse_children(child, storage, src_files);
            break;
        }
        case dw::tag::class_type:
        case dw::tag::enumeration_type:
        case dw::tag::structure_type:
        case dw::tag::union_type:
        case dw::tag::subprogram: {
            auto it = child.attributes().find(dw::attribute_t::decl_file);
            if (it == child.attributes().end()) {
                break;
            }
            const auto &decl_file = src_files.at(it->get<int>());

            it = child.attributes().find(dw::attribute_t::name);
            if (it == child.attributes().end()) {
                break;
            }
            auto name = it->get<std::string>();

            storage[decl_file].emplace(name);
            spdlog::warn("{} {}", decl_file, name);
            break;
        }
        default:
            break;
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

    int i = 0;
    file_entry_storage storage;
    std::string source_dir;
    for (const auto &cu : debug) {
        auto &cu_die = cu.die();
        auto die_name = cu_die.find(dw::attribute_t::name)->get<std::string>();
        auto comp_dir = cu_die.find(dw::attribute_t::comp_dir)->get<std::string>();
        spdlog::info("[{:<4}] {}", ++i, die_name);

        if (source_dir.empty()) {
            source_dir = die_name;
        }
        source_dir = posixpath::commonpath({die_name, comp_dir, source_dir});

        auto src_files = cu_die.src_files();
        parse_children(cu_die, storage, src_files);
    }

    for (const auto &[filepath, content] : storage) {
        if (posixpath::commonpath({filepath, source_dir}) != source_dir) {
            spdlog::warn("skipping {}", filepath);
        }
    }
    return 0;
}
