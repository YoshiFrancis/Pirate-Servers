cmake_minimum_required(VERSION 3.29)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(Server Server.cpp ServerConn.cpp)

target_include_directories(Server PRIVATE ${PROJECT_SOURCE_DIR}/../include/server ${PROJECT_SOURCE_DIR}/../include)
