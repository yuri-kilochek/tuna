from conans import ConanFile, tools, CMake

class TunaConan(ConanFile):
    name = 'tuna'
    version = tools.load('VERSION').strip()
    settings = 'os', 'compiler', 'build_type', 'arch'
    exports = (
        'VERSION',
    )
    exports_sources = (
        'CMakeLists.txt',
        'cmake/*',
        'include/*',
        'src/*',
    )
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
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)

