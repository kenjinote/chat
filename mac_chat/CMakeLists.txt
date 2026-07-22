set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")
cmake_minimum_required(VERSION 3.14)
project(RetroChat VERSION 1.0 LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_OBJCXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -fobjc-arc")

# Find Apple frameworks
find_library(APPKIT_LIBRARY AppKit)
find_library(AVFOUNDATION_LIBRARY AVFoundation)
find_library(COREMEDIA_LIBRARY CoreMedia)
find_library(COREVIDEO_LIBRARY CoreVideo)
find_library(QUARTZCORE_LIBRARY QuartzCore)
find_library(VIDEOTOOLBOX_LIBRARY VideoToolbox)
find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
find_library(COREAUDIO_LIBRARY CoreAudio)



set(SOURCES
    main.mm
    ChatNetwork.cpp
    MacAudioEngine.mm
    MacVideoEngine.mm
    json11.cpp
)

file(GLOB_RECURSE RESOURCE_FILES "Resources/*")

add_executable(RetroChat MACOSX_BUNDLE ${SOURCES} ${RESOURCE_FILES} Info.plist)

set_target_properties(RetroChat PROPERTIES
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    RESOURCE "${RESOURCE_FILES}"
)
target_compile_options(RetroChat PRIVATE -fobjc-arc -Wno-availability -Wno-unguarded-availability -Wno-unguarded-availability-new -Wno-deprecated-declarations -Wno-unavailable-declarations)

target_link_libraries(RetroChat PRIVATE
    ${APPKIT_LIBRARY}
    ${AVFOUNDATION_LIBRARY}
    ${COREMEDIA_LIBRARY}
    ${COREVIDEO_LIBRARY}
    ${QUARTZCORE_LIBRARY}
    ${VIDEOTOOLBOX_LIBRARY}
    ${AUDIOTOOLBOX_LIBRARY}
    ${COREAUDIO_LIBRARY}
)
