cmake_minimum_required(VERSION 3.29) 
set(CMAKE_CXX_STANDARD_REQUIRED 20)
set(CMAKE_CXX_STANDARD 20)
project(Pirates 
				VERSION 3.29
				LANGUAGES CXX
)

include_directories(~/Documents/ASIO/asio-1.30.2/include include)

add_executable(Pirates main.cpp)
