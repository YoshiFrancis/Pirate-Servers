cmake_minimum_required(VERSION 3.10)
project(PirateServers)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)

add_executable(MainClient client_main.cpp)
target_link_libraries(MainClient Client)

add_executable(MainServer server_main.cpp)
target_link_libraries(MainServer Server)
