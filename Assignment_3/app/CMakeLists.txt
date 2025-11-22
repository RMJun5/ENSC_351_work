# Build the app, using the HAL
include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(beatbox ${MY_SOURCES})
find_package(ALSA REQUIRED)

# Make use of the HAL library
target_link_libraries(beatbox LINK_PRIVATE hal)
target_link_libraries(beatbox LINK_PRIVATE asound)
target_link_libraries(beatbox LINK_PRIVATE audio_lib)


# Copy executable to final location (change `beatbox` to project name as needed)
add_custom_command(TARGET beatbox POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:beatbox>"
     "~/ENSC_351/public/myApps/beatbox" 
  COMMENT "Copying ARM executable to public NFS directory")



# debugging print messages

message(STATUS "Audio directory: ${CMAKE_SOURCE_DIR}/audio")
message(STATUS "Linking audio directory from: ${CMAKE_SOURCE_DIR}/audio")
message(STATUS "Linking audio directory to: ${CMAKE_CURRENT_BINARY_DIR}/audio")