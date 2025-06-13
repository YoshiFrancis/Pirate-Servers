cmake_minimum_required(VERSION 3.29) 
set(CMAKE_CXX_STANDARD_REQUIRED 20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(Pirates 
				VERSION 3.29
				LANGUAGES CXX
)

include_directories(~/Documents/ASIO/asio-1.30.2/include)
add_definitions(-DASIO_STANDALONE)

add_subdirectory(server)

add_executable(PiratesServer server_main.cpp)

target_link_libraries(PiratesServer PRIVATE Server)
