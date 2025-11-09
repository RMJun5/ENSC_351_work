# CMake Build Configuration for root of project

cmake_minimum_required(VERSION 3.18)
project(light_sampler C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler options (inherited by sub-folders)

# add_compile_options(-Wall -Werror -Wpedantic -Wextra) 
# add_compile_options(-fdiagnostics-color)

# --- Toolchain auto-detect ---
if(NOT CMAKE_TOOLCHAIN_FILE AND NOT CMAKE_CROSSCOMPILING)
    set(TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/aarch64-toolchain.cmake")
    message(STATUS "Setting toolchain file to /aarch64-toolchain.cmake...")
    if(EXISTS ${TOOLCHAIN_FILE})
        message(STATUS "Auto-detecting toolchain file: ${TOOLCHAIN_FILE}")
    else()
        message(STATUS "No toolchain file found, using default compiler")
    endif()
endif()


message(STATUS "Cross-compiling with sysroot: ${CMAKE_CROSSCOMPILING}")

# --- Sysroot support ---
if(CMAKE_CROSSCOMPILING AND DEFINED CMAKE_SYSROOT)
    message(STATUS "Cross-compiling with sysroot: ${CMAKE_SYSROOT}")
    set(ENV{PKG_CONFIG_SYSROOT_DIR} "~/ENSC_351/public/byai-sysroot")          
    set(ENV{PKG_CONFIG_PATH} "~/ENSC_351/public/byai-sysroot/usr/pkgconfig")
    set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
endif()

    


# --- Ensure pkg-config is available ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)

# Enable address sanitizer
# # (Comment this out to make your code faster)
# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

# Enable PThread library for linking
add_compile_options(-pthread)
add_link_options(-pthread)

# What folders to build
add_subdirectory(hal)  
add_subdirectory(app)

# --- Diagnostics ---
message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "System processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER}")




