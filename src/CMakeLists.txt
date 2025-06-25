cmake_minimum_required(VERSION 3.29) 
set(CMAKE_CXX_STANDARD_REQUIRED 20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ZeroMQ)
find_package(cppzmq)

add_library(Helper INTERFACE zhelpers.hpp)
add_library(MyZMQ INTERFACE)
target_link_libraries(MyZMQ INTERFACE
    Helper
    libzmq-static
    cppzmq-static
)
