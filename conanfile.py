from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout

import os

class Pkg(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def s2str(self, name):
        return str(getattr(self.settings, name))

    def s2tuple(self, first, second):
        return "{0}-{1}".format(self.s2str(first), self.s2str(second))

    def requirements(self):
        self.requires("cppzmq/4.10.0")
        self.tool_requires("cmake/[>=3.25]")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def layout(self):
        derived_dir = os.path.join(
                "build",
                self.s2tuple("os", "arch"),
                self.s2tuple("compiler", "build_type")
                )
        cmake_layout(self, build_folder=derived_dir)
