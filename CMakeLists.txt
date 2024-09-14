cmake_minimum_required(VERSION 3.15...3.30)

project(MicArray VERSION 1.0)

# Find ALSA package
find_package(ALSA REQUIRED)

# Add the executable
add_executable(MicArray main.c mic_array.c)

# Include the ALSA headers
target_include_directories(MicArray PUBLIC "${PROJECT_SOURCE_DIR}" ${ALSA_INCLUDE_DIRS})

# Link the ALSA library
target_link_libraries(MicArray PRIVATE ${ALSA_LIBRARIES})
