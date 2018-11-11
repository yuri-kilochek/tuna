from conans import ConanFile, tools, CMake

class InterfaceptorConan(ConanFile):
    name = 'interfaceptor'
    version = tools.load('VERSION').strip()
    requires = (
        'boost/1.68.0@conan/stable',
    )
    exports = (
        'VERSION',
    )
    exports_sources = (
        'CMakeLists.txt',
        'include/*',
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

    def package(self):
        self.copy('*', 'include', 'include')
