cmake_minimum_required(VERSION 3.15...3.30)

project(MicArray VERSION 1.0)

# Add the executable
add_executable(MicArray main.c mic_array.c)

# Include ALSA headers
target_include_directories(MicArray PUBLIC "${PROJECT_SOURCE_DIR}")

# Manually link to ALSA
target_link_libraries(MicArray PRIVATE asound)
