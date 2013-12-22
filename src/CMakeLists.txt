find_package (glfw REQUIRED)
find_package (PNG REQUIRED)
find_package (GLM REQUIRED)
find_package (OPENGL REQUIRED)

include_directories (${GLFW_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/glcorew ${PNG_INCLUDE_DIRS} ${GLM_INCLUDE_DIR})

file (GLOB PBF_SOURCES *.cpp)

add_executable (pbf ${PBF_SOURCES})

target_link_libraries (pbf ${GLFW_LIBRARY} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} glcorew)

set (USE_CXX11 YES CACHE BOOL "Enable C++11")
if (USE_CXX11)
set_property (TARGET pbf PROPERTY COMPILE_FLAGS -std=c++11)
endif (USE_CXX11)
