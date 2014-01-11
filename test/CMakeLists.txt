find_package (glfw REQUIRED)
find_package (OpenGL REQUIRED)

include_directories (${GLFW_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/glcorew)

file (GLOB TEST_SOURCES *.cpp)

add_executable (test ${TEST_SOURCES})

target_link_libraries (test ${GLFW_LIBRARY} ${OPENGL_LIBRARIES} glcorew)
