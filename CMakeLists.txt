cmake_minimum_required(VERSION 3.0)

project(CAN2NET)

set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)
find_library(LIBRT rt)

file(GLOB SOURCES "src/*.cpp")

add_executable(can2net ${SOURCES})
target_include_directories(can2net PRIVATE include)
target_link_libraries(can2net pthread rt)