from conans import ConanFile, tools, CMake
import os

class ExampleConan(ConanFile):
    generators = (
        'cmake_find_package',
        'cmake_paths',
    )
    short_paths = True
    no_copy_source = True

    def build(self):
        cmake = CMake(self)
        cmake.configure(defs={
            'CMAKE_TOOLCHAIN_FILE': 'conan_paths.cmake'
        })
        cmake.build()

    def test(self):
        self.run('sudo .' + os.sep + 'example')
