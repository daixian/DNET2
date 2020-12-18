cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 定义UNICODE会是TCHAR为wchar
# add_definitions(-D UNICODE)
if(WIN32)
    add_definitions(-D _WINDOWS)
    add_definitions(-D _USERDLL)
endif()

# 先自动搜索文件添加算了
FILE(GLOB_RECURSE SOURCE_LIST "*.cpp" "*.h" "*.hpp" "*.c" "*.cc")

# 这个函数在VS工程里生成一个文件路径
function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        #这个表示在VS里划分一个组
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction(assign_source_group)

assign_source_group(${SOURCE_LIST})
message(STATUS "SOURCE_LIST = ${SOURCE_LIST}")

# 增加lib搜索文件夹,注意需要在add_executable之前
# https://stackoverflow.com/questions/28597351/how-do-i-add-a-library-path-in-cmake
# set(NATNETSDK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../NatNetSDK/lib/x64")
# link_directories(${NATNETSDK_LIB_DIR})

# 生成可执行文件
add_library(${SRC_NAME}.so
            SHARED
            ${SOURCE_LIST})

find_package (Threads)

message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message(STATUS "CONAN_LIBS = ${CONAN_LIBS}")


target_include_directories(${SRC_NAME}.so PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_link_libraries(${SRC_NAME}.so PRIVATE ${CONAN_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${SRC_NAME})

# if(WIN32)
#     #链接某些自定义的依赖的库
#     target_include_directories(${SRC_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../DShow)
#     target_link_libraries(${SRC_NAME} PRIVATE baseclasses)
# endif()

# 全都安装到根目录
install(TARGETS ${SRC_NAME}
       RUNTIME DESTINATION "./"
       LIBRARY DESTINATION "./"
       ARCHIVE DESTINATION "./"
)
# 安装头文件
# install(FILES "dlog.h" DESTINATION "./")
# install(FILES "../UnitTest/DLog.cs" DESTINATION "./")

# Configuration
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
