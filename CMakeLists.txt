cmake_minimum_required(VERSION 3.31)
project(ChipEight)

set(CMAKE_CXX_STANDARD 20)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_library(ChipEight STATIC interpreter.cpp)

add_executable(main main.cpp)

target_link_libraries(main PRIVATE ChipEight SFML::Graphics SFML::Window SFML::System)

add_executable(tests tests.cpp)

target_link_libraries(tests PRIVATE ChipEight SFML::Graphics SFML::Window SFML::System)
