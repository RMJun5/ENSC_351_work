# ALSA support 
find_package(ALSA REQUIRED) 
target_link_libraries(wave_player_cmake LINK_PRIVATE asound)

add_custom_command(TARGET wave_player_cmake POST_BUILD 
COMMAND "${CMAKE_COMMAND}" -E copy_directory "
    ${CMAKE_SOURCE_DIR}/wave-files" 
    "~/ensc351/public/myApps/wave-files"
COMMENT "Copying WAVE files to public NFS directory")