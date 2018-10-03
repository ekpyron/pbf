set(SOURCES main.cpp glfw.h Context.cpp Context.h common.h Swapchain.cpp Swapchain.h Renderer.cpp Renderer.h Cache.h objects/RenderPass.h objects/DescriptorOrder.h objects/VkOrder.h objects/RenderPass.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC glfw fmt vulkan spdlog glm)
