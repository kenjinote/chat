cmake_minimum_required(VERSION 3.22.1)

project("android_chat")

# Set Opus directory
set(OPUS_DIR "../../../../../../../opus")

# Add Opus as a subdirectory (if you want to compile from source)
# To keep it lightweight and fast, we can just build the required opus files 
# or use the existing CMakeLists from Opus if compatible.
# For simplicity, we just use the opus cmake build system.
set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "Build shared library" FORCE)
set(OPUS_BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "Build programs" FORCE)
add_subdirectory(${OPUS_DIR} opus_build EXCLUDE_FROM_ALL)

add_library(
        opus_wrapper
        SHARED
        opus_wrapper.cpp)

target_include_directories(opus_wrapper PRIVATE ${OPUS_DIR}/include)

find_library(
        log-lib
        log)

target_link_libraries(
        opus_wrapper
        opus
        ${log-lib})
