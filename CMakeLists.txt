if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(pbf LANGUAGES CXX VERSION 0.0.0)

cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_subdirectory(contrib/fmt)
add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_subdirectory(contrib/spdlog)
add_subdirectory(contrib/glm)
add_subdirectory(contrib/crampl)

add_subdirectory(src)
