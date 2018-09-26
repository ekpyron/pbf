set(SOURCES main.cpp glfw.h Context.cpp Context.h common.h)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC glfw fmt vulkan spdlog)
