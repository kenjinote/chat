cmake_minimum_required(VERSION 3.20)
project(chat_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages using vcpkg
if(WIN32)
    find_package(asio CONFIG REQUIRED)
    find_package(redis++ CONFIG REQUIRED)
endif()

# Executable
add_executable(chat_server main.cpp json11.cpp)

# Include directories
target_include_directories(chat_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries
if(WIN32)
    target_link_libraries(chat_server PRIVATE asio::asio redis++::redis++)
endif()

# Link against pthread on Linux
if(UNIX)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(chat_server PRIVATE Threads::Threads redis++ hiredis crypto)
endif()
