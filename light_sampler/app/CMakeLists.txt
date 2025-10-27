# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(out ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(out LINK_PRIVATE hal)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET out POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:out>"
     "~/ENSC_351/public/myApps/ENSC_351_work/light_sampler" 
  COMMENT "Copying ARM executable to public NFS directory")
