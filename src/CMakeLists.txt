cmake_minimum_required(VERSION 3.29) 
set(CMAKE_CXX_STANDARD_REQUIRED 20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(cppzmq)

add_subdirectory(client)

add_library(MyZMQ INTERFACE)
target_link_libraries(MyZMQ INTERFACE
    cppzmq-static
)

target_include_directories(MyZMQ INTERFACE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(Client MyZMQ)
