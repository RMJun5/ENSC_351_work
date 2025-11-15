# Build the app, using the HAL

# include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(out ${MY_SOURCES})

target_include_directories(out PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/include   # app headers
   ${CMAKE_SOURCE_DIR}/hal/include       # HAL headers
   ${LIBGPIOD_INCLUDE_DIRS}              # libgpiod from sysroot

)

# Make use of the HAL library
target_link_libraries(out LINK_PRIVATE 
   hal
   ${LIBGPIOD_LIBRARIES}
)

# --- Compiler warnings / sanitizers ---
target_compile_options(out PRIVATE
   -Wall -Werror -Wpedantic -Wextra -fdiagnostics-color -fsanitize=address -pthread
)
target_link_options(out PRIVATE
   -fsanitize=address -pthread
)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET out POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:out>"
     "~/ENSC_351/public/" 
  COMMENT "Copying ARM executable to public NFS directory")
