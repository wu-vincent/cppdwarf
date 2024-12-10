cmake_minimum_required(VERSION 3.15)
project(cppdwarf LANGUAGES CXX)

option(CPPDWARF_EXTERNAL_LIBDWARF "Use an external libdwarf (via find_package)" OFF)

if (CPPDWARF_EXTERNAL_LIBDWARF)
    find_package(libdwarf REQUIRED)
else ()
    include(FetchContent)
    FetchContent_Declare(
            libdwarf
            GIT_REPOSITORY https://github.com/davea42/libdwarf-code.git
            GIT_TAG v0.11.1
    )
    FetchContent_MakeAvailable(libdwarf)
endif ()

add_library(cppdwarf INTERFACE)
add_library(cppdwarf::cppdwarf ALIAS cppdwarf)
target_include_directories(cppdwarf INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(cppdwarf INTERFACE libdwarf::libdwarf)

add_subdirectory(examples/dwarf2cpp)
