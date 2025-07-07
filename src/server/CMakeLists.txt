cmake_minimum_required(VERSION 3.29)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(Server 
    Cabin.cpp 
    CabinContainer.cpp 
    CrewmateContainer.cpp
    Crewmember.cpp 
    Server.cpp 
    Shipbridge.cpp 
    ShipbridgeContainer.cpp 
    Shipdeck.cpp
    defines.cpp
)

target_include_directories(Server PRIVATE ${CMAKE_SOURCE_DIR}/include/server)
