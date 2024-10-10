cmake_minimum_required(VERSION 3.15...3.30)

project(MicArray VERSION 1.0)

add_executable(MicArray main.c mic_array.c)

target_include_directories(MicArray PUBLIC "${PROJECT_SOURCE_DIR}")

# Find ALSA library
find_library(ALSALIB asound REQUIRED)

# Find libusb-1.0 library
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

# Find GStreamer and GLib packages
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Link the libraries, including pthread, GStreamer, and GLib
target_link_libraries(MicArray PRIVATE ${ALSALIB} ${LIBUSB_LIBRARIES} ${GST_LIBRARIES} ${GLIB_LIBRARIES} pthread)

# Include libusb, GStreamer, and GLib headers
target_include_directories(MicArray PRIVATE ${LIBUSB_INCLUDE_DIRS} ${GST_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})

