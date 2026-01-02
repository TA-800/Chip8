cmake_minimum_required(VERSION 3.31)
project(ChipEight)

set(CMAKE_CXX_STANDARD 20)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_executable(ChipEight main.cpp)

target_link_libraries(ChipEight PRIVATE SFML::Graphics SFML::Window SFML::System)
