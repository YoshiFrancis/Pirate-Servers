cmake_minimum_required(VERSION 3.29) 
set(CMAKE_CXX_STANDARD_REQUIRED 20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(Client STATIC Client.cpp)

target_include_directories(Client PRIVATE ${CMAKE_SOURCE_DIR}/include/client)
