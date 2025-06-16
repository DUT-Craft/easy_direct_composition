cmake_minimum_required(VERSION 3.10)
project(easy_direct_composition VERSION 1.0.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 强制使用 UTF-8 编码
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
# 启用 Unicode 支持
add_compile_definitions(UNICODE _UNICODE)

# 创建库目标
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

# 设置库的别名，便于在父项目中使用
add_library(EasyDirectComposition::easy_direct_composition ALIAS easy_direct_composition)

# 设置包含目录
target_include_directories(easy_direct_composition
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接系统库
target_link_libraries(easy_direct_composition
    PUBLIC
        d3d11
        dcomp
        d2d1
        dwrite
        dxgi
        ole32
        uuid
        version
)

# 设置编译特性
target_compile_features(easy_direct_composition
    PUBLIC
        cxx_std_20
    PRIVATE
        c_std_11
)

# 可选：安装配置（暂时注释掉，专注于基本功能）
# include(GNUInstallDirs)
# include(CMakePackageConfigHelpers)
#
# install(TARGETS easy_direct_composition
#     EXPORT EasyDirectCompositionTargets
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )
