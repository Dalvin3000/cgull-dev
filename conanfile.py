from conans import ConanFile, tools #, CMake
from conan.tools.cmake import CMake
from conan.tools.cmake import CMakeToolchain
import os

class CGullConan(ConanFile):
    name = 'CGull'
    version = '0.1.0'
    license = 'https://github.com/Dalvin3000/cgull-dev/blob/master/LICENSE'
    url = 'https://github.com/Dalvin3000/cgull-dev'
    description = 'Async primitives for C++20 inspired by BlueBird.js'
    settings = 'os', 'compiler', 'build_type', 'arch'
    generators = 'cmake_find_package_multi'
    options = {
        'shared': [True, False],
        'cgull_build_tests': [True, False],
        'cgull_with_boost': [True, False],
    }
    default_options = {
        'shared': False,
        'cgull_build_tests': False,
        'cgull_with_boost': False,
    }
    revision_mode = 'scm'
    exports_sources = 'src/*', 'include/*', 'CMakeLists.txt'

    def source(self):
        tools.download('https://github.com/Dalvin3000/cgull-dev/archive/master.zip', 'cgull.zip')
        tools.unzip('cgull.zip')
        os.unlink('cgull.zip')
        os.system('mv cgull-dev-master/* ./')

    def requirements(self):
        if self.options.cgull_with_boost:
            self.requires('boost/1.75.0')

    def build_requirements(self):
        if self.options.cgull_build_tests:
            self.build_requires('gtest/1.10.0')

    def generate(self):
        tc = CMakeToolchain(self)

        tc.variables['CGULL_BUILD_TESTS'] = self.options.cgull_build_tests
        tc.variables['CGULL_WITH_BOOST'] = self.options.cgull_with_boost
        tc.variables['CMAKE_FIND_PACKAGE_PREFER_CONFIG'] = True
        tc.variables['CMAKE_CONFIGURATION_TYPES'] = self.settings.build_type

        tc.generate()

    def configure(self):
        pass

    def imports(self):
        pass

    def build(self):
        cmake = CMake(self)

        if self.should_configure:
            cmake.configure()

        if self.should_build:
            cmake.build()

        if self.should_install:
            cmake.install()

        if self.should_test:
            cmake.test()

    def package(self):
        cmake = self._cmake
        cmake.install()

    def package_info(self):
        self.cpp_info.name = 'CGull'
        self.cpp_info.components['CGull'].name = 'CGull'
        self.cpp_info.components['CGull'].libs = ['CGull']
