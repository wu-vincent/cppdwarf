#include <argparse/argparse.hpp>
#include <cppdwarf/cppdwarf.hpp>
#include <spdlog/spdlog.h>

namespace dw = cppdwarf;

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
    for (const auto &cu : debug) {
        auto &die = cu.die();
        for (const auto &attribute : die.attributes()) {
            spdlog::info("[cu/{}] attribute: {}", i, attribute.name());
        }
        i++;
    }
    spdlog::info("{} Compilation Units", i);

    return 0;
}
