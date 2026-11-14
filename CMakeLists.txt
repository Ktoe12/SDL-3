cmake_minimum_required(VERSION 4.0)
project(SDL3_Project_SelfContained)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(SDL EXCLUDE_FROM_ALL)

add_executable(SDL3_Project_SelfContained main.cpp
        main.cpp)

target_link_libraries(SDL3_Project_SelfContained PRIVATE SDL3::SDL3)