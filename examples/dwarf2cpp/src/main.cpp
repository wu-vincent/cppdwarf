#include <argparse/argparse.hpp>
#include <cppdwarf/cppdwarf.hpp>

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
        i++;
    }
    printf("%d Compilation Units\n", i);

    return 0;
}
