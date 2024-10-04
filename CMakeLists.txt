cmake_minimum_required(VERSION 3.15...3.30)

project(MicArray VERSION 1.0)

add_executable(MicArray main.c mic_array.c)

target_include_directories(MicArray PUBLIC "${PROJECT_SOURCE_DIR}")

# Find ALSA library
find_library(ALSALIB asound REQUIRED)

# Find libusb-1.0 library
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

# Link the libraries, including pthread
target_link_libraries(MicArray PRIVATE ${ALSALIB} ${LIBUSB_LIBRARIES} pthread)

# Include libusb headers
target_include_directories(MicArray PRIVATE ${LIBUSB_INCLUDE_DIRS})

