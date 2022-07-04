cmake_minimum_required(VERSION 3.11)
project(pbf LANGUAGES CXX VERSION 0.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")
set(CMAKE_CXX_STANDARD 20)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_subdirectory(contrib/fmt)
add_definitions(-DSPDLOG_FMT_EXTERNAL -DVULKAN_HPP_NO_STRUCT_CONSTRUCTORS)
add_subdirectory(contrib/spdlog)
add_subdirectory(contrib/glm)
add_subdirectory(contrib/crampl)

add_subdirectory(resources/shaders)
add_subdirectory(pbf)
