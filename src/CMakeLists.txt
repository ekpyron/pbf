find_package (glfw3 REQUIRED)
find_package (PNG REQUIRED)
find_package (GLM REQUIRED)
find_package (OpenGL REQUIRED)
find_package (Threads REQUIRED)

include_directories (${GLFW_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/glcorew ${PNG_INCLUDE_DIRS} ${GLM_INCLUDE_DIR})

if (WIN32)
    set (CMAKE_EXE_LINKER_FLAGS "-static")
endif (WIN32)

file (GLOB PBF_SOURCES *.cpp)

add_executable (pbf ${PBF_SOURCES})

target_link_libraries (pbf glfw ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} glcorew spdlog Threads::Threads)
add_dependencies(pbf shaders-target textures-target)
