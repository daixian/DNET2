# coding: utf-8
from conans import ConanFile, CMake, tools

import os
import sys
import io
# sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='gbk')

os.system("chcp 65001")


class DNETConan(ConanFile):
    name = "dnet"
    version = "1.0.0"
    license = "<Put the package license here>"
    author = "daixian<amano_tooko@qq.com>"
    url = "https://github.com/daixian/DNET2"
    description = "通信库"
    topics = ("unity", "net", "daixian")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "build_test": [True, False]}
    default_options = {"shared": False,
                       "build_test": True,
                       "dlog:shared": False}
    generators = "cmake"
    exports_sources = "src/*"

    def requirements(self):
        self.requires("dlog/[>=2.6.0]@daixian/stable")
        self.requires("xuexuesharp/[>=0.0.16]@daixian/stable")
        self.requires("xuexuejson/[>1.1.0]@daixian/stable")

    def _configure_cmake(self):
        '''
        转换python的设置到CMake
        '''
        cmake = CMake(self)
        if self.settings.os == "Android":
            # 如果是Android那么要编译成so
            cmake.definitions["DNET_BUILD_SHARED"] = True
        if self.settings.os == "iOS":
            # 如果是iOS那么要编译成.a
            cmake.definitions["DNET_BUILD_SHARED"] = False
        else:
            cmake.definitions["DNET_BUILD_SHARED"] = self.options.shared
        cmake.definitions["DNET_BUILD_TESTS"] = self.options.build_test
        return cmake

    def build(self):
        print("进入了build...")
        cmake = self._configure_cmake()
        cmake.configure(source_folder="src")
        cmake.build()

        # Explicit way:
        # self.run('cmake %s/hello %s'
        #          % (self.source_folder, cmake.command_line))
        # self.run("cmake --build . %s" % cmake.build_config)

    def package(self):
        self.copy("*.h", dst="include", src="src")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["DNET"]
