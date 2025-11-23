# CMakeList.txt for HAL
#   Build a library (`hal`) which exposes the header files as "hal/*.h"
#   Use header as: #include "hal/button.h"
include_directories(hal/include)
file(GLOB MY_SOURCES "src/*.c")

add_library(hal STATIC ${MY_SOURCES})

# Add periodtimer.h
target_include_directories(hal
    BEFORE
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# --- Find libgpiod ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGPIOD REQUIRED libgpiod)

# --- Include directories ---
target_include_directories(hal
    BEFORE
    PUBLIC
        ${LIBGPIOD_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
 
# --- Link against libgpiod ---
target_link_libraries(hal
    PUBLIC 
        ${LIBGPIOD_LIBRARIES}
        m          # libm for math functions
        pthread    # pthread
)

# --- Diagnostics ---
message(STATUS "libgpiod include dirs: ${LIBGPIOD_INCLUDE_DIRS}")
message(STATUS "Using libgpiod version: ${LIBGPIOD_VERSION}")
message(STATUS "libgpiod libraries: ${LIBGPIOD_LIBRARIES}")

