cmake_minimum_required(VERSION 3.11)
project(pbf LANGUAGES CXX VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

set(FMT_TEST OFF)
add_subdirectory(contrib/fmt)

add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_subdirectory(contrib/spdlog)
add_subdirectory(contrib/glm)
add_subdirectory(contrib/crampl)

add_subdirectory(src)
