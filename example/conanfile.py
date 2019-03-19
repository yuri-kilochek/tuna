from conans import ConanFile, CMake
import os

class ExampleConan(ConanFile):
    settings = 'os', 'compiler', 'build_type', 'arch'
    generators = (
        'cmake_find_package',
        'cmake_paths',
    )
    requires = (
        ('boost/1.68.0@conan/stable'),        
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
        if self.settings.os == 'Linux':
            self.run('sudo valgrind .' + os.sep + 'example')
        elif self.settings.os == 'Windows':
            self.run(str(self.settings.build_type) + os.sep + 'example.exe')
