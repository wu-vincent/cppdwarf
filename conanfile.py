import os

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy


class CppDwarfConan(ConanFile):
    name = "cppdwarf"
    homepage = "https://github.com/wu-vincent/cppdwarf"
    description = "Header-only C++ library wrapping libdwarf"
    topics = ("debug", "dwarf", "header-only")
    license = "MIT"
    package_type = "header-library"
    settings = "os", "compiler", "build_type", "arch"
    no_copy_source = True

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("libdwarf/0.11.1")

    def package_id(self):
        self.info.clear()

    def validate(self):
        check_min_cppstd(self, 11)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["CPPDWARF_EXTERNAL_LIBDWARF"] = "ON"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(
            self,
            "LICENSE",
            dst=os.path.join(self.package_folder, "licenses"),
            src=self.source_folder,
        )
        copy(
            self,
            src=os.path.join(self.source_folder, "include"),
            pattern="*.hpp",
            dst=os.path.join(self.package_folder, "include"),
        )

    def package_info(self):
        self.cpp_info.components["cppdwarf"].libs = []
        self.cpp_info.components["cppdwarf"].libdirs = []
        self.cpp_info.components["cppdwarf"].set_property("cmake_target_name", "cppdwarf::cppdwarf")
        self.cpp_info.components["cppdwarf"].requires = ["libdwarf::libdwarf"]
