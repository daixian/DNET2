# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
#
cmake_minimum_required(VERSION 3.0)

# 在上面设置工具链之后下面要立马project命令
project(DNET VERSION 3.0.0 LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 14)

message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")

# 在有了project之后需要第一时间搞个conan设置吧
if(NOT DEFINED CONAN_BUILD_INFO_PATH)
    # 如果没有设置过
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif()
# conan_set_vs_runtime()

# # 设置是否编译Test,现在把Test移出去了,移到了test_package
# option(DNET_BUILD_TESTS "DNET_BUILD_TESTS" ON)
# message(STATUS "DNET_BUILD_TESTS = ${DNET_BUILD_TESTS}")

# 设置是否编译成共享库
option(DNET_BUILD_SHARED "DNET_BUILD_SHARED" OFF)
message(STATUS "DNET_BUILD_SHARED = ${DNET_BUILD_SHARED}")

add_definitions(-D DLOG_UTF8)
# add_definitions(-D _NO_ASYNCRTIMP)

# 设置生成一个VS工程里的文件夹
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 修改一些默认值 Use this snippet *after* PROJECT(xxx):
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install CACHE PATH "Install directory" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# 设置文件输出的目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 这个模块的名字
set(SRC_NAME "DNET")

set(DNET_BUILD_SHARED TRUE)

# 如果构建共享库
if(DNET_BUILD_SHARED)
    if(WIN32)
        add_definitions(-D DNET_DLL)
        add_definitions(-D DNET_FUNC_EXPORTS)
    endif()
endif()

add_subdirectory(${SRC_NAME})
# add_subdirectory(App)

# 如果构建共享库
if(DNET_BUILD_SHARED)
    add_subdirectory(Shared)
endif()

# if(DNET_BUILD_TESTS)
# # 使能测试
# enable_testing()
# include(CTest)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Test)
# endif()