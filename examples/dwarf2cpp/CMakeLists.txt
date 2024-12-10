cmake_minimum_required(VERSION 3.15)
project(dwarf2cpp LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
        argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

# From https://github.com/nico/demumble
# demumble is a project that extracts small portion of LLVM code that is responsible for C++ symbol demangling.
add_library(llvm-demangle
        third_party/llvm/lib/Demangle/Demangle.cpp
        third_party/llvm/lib/Demangle/ItaniumDemangle.cpp
        third_party/llvm/lib/Demangle/DLangDemangle.cpp
        third_party/llvm/lib/Demangle/MicrosoftDemangle.cpp
        third_party/llvm/lib/Demangle/MicrosoftDemangleNodes.cpp
        third_party/llvm/lib/Demangle/RustDemangle.cpp)
add_library(llvm-demangle::llvm-demangle ALIAS llvm-demangle)
target_include_directories(llvm-demangle PUBLIC third_party/llvm/include)

add_executable(dwarf2cpp src/main.cpp)
target_link_libraries(dwarf2cpp PRIVATE cppdwarf::cppdwarf
        argparse::argparse)

