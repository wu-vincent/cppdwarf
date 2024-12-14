cmake_minimum_required(VERSION 3.15)
project(file2types LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
        argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git
        GIT_TAG v3.1
)
FetchContent_MakeAvailable(argparse)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.0
)
FetchContent_MakeAvailable(spdlog)
FetchContent_Declare(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(file2types main.cpp)
target_link_libraries(file2types PRIVATE cppdwarf::cppdwarf
        argparse::argparse
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

