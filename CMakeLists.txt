project(pbf LANGUAGES CXX)

cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_subdirectory(contrib/fmt)
add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_subdirectory(contrib/spdlog)

add_subdirectory(src)
