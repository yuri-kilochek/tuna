from conans import ConanFile, tools, CMake

class TunaConan(ConanFile):
    name = 'tuna'
    version = tools.load('VERSION').strip()
    settings = (
        'os',
        'compiler',
        'build_type',
        'arch',
    )
    options = {
        'shared': [False, True],
    }
    default_options = {
        'shared': True,        
    }
    exports = (
        'VERSION',
    )
    exports_sources = (
        'CMakeLists.txt',
        'include/*',
        'src/*',
    )
    generators = (
        'cmake_find_package',
        'cmake_paths',
    )
    short_paths = True
    no_copy_source = True

    def requirements(self):
        if self.settings.os == 'Linux':
            self.requires('libnl/3.4.0@yurah/stable')

    def build(self):
        cmake = CMake(self)
        cmake.configure(defs={
            'CMAKE_TOOLCHAIN_FILE': 'conan_paths.cmake'
        })
        cmake.build()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
        if self.options.shared:
            self.cpp_info.defines.append('TUNA_IMPORT')
