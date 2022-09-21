cmake_minimum_required(VERSION 3.8)

project(link_scanner CXX)
set(CMAKE_CXX_STANDARD 14)


find_package(Threads REQUIRED)

add_executable(scanner scanner.cpp)
target_link_libraries(scanner PRIVATE Threads::Threads)
