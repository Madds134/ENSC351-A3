# Build the beatbox application, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")

add_executable(beatbox ${MY_SOURCES})
target_link_libraries(beatbox LINK_PRIVATE hal)

# ALSA support (finds the arm64 library installed on the host)
find_package(ALSA REQUIRED)
target_link_libraries(beatbox LINK_PRIVATE asound)

# Deploy the program, drum sounds, and NodeJS server to the NFS folder
set(MY_APPS_DIR "$ENV{HOME}/ensc351/public/myApps")
add_custom_command(TARGET beatbox POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy
        "$<TARGET_FILE:beatbox>" "${MY_APPS_DIR}/beatbox"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/wave-files" "${MY_APPS_DIR}/beatbox-wav-files"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/beatbox-server" "${MY_APPS_DIR}/beatbox-server-copy"
    COMMENT "Deploying beatbox, wave files, and server to NFS"
)
