cmake_minimum_required(VERSION 3.15...3.30)

project(MicArray VERSION 1.0)

add_executable(MicArray main.c mic_array.c)

target_include_directories(MicArray PUBLIC "${PROJECT_SOURCE_DIR}")

find_library(ALSALIB asound REQUIRED)

target_link_libraries(MicArray PRIVATE ${ALSALIB})
