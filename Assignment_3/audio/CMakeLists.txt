# audio/CMakeLists.txt

# Find ALSA
find_package(ALSA REQUIRED)
file(GLOB AUDIO_SOURCES "src/*.c")
add_library(audio_lib STATIC ${AUDIO_SOURCES})

# Create a static library from your audio code
# (Replace audio.c with the actual .c files you have)
target_include_directories(audio_lib 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /home/kbph/ENSC_351/public/myApps/ENSC_351_work/Assignment_3/app/include
    
)

# Link library with ALSA
target_link_libraries(audio_lib
    PRIVATE
    asound
)

# Copy WAVE files to your NFS/public directory after build
add_custom_command(
    TARGET audio_lib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/audio/wave-files
            ~/ENSC_351/public/myApps/wave-files
    COMMENT "Copying WAVE files to public NFS directory"
)
