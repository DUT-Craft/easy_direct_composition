cmake_minimum_required(VERSION 3.10)
project(easy_direct_composition)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 强制使用 UTF-8 编码
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
# 启用 Unicode 支持
add_compile_definitions(UNICODE _UNICODE)

# 添加输出库
add_library(easy_direct_composition
    src/obj_tree.c
    src/obj_tree.h
    src/obj_helper.cpp
    src/obj_helper.h
    src/dc_env.cpp
    src/dc_env.h
    src/dc_surface.cpp
    src/dc_surface.h
)

# 链接库
target_link_libraries(easy_direct_composition
    d3d11
    dcomp
    d2d1
    dwrite
    dxgi
    ole32
    uuid
    version
)
