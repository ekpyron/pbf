
set(SOURCES main.cpp glfw.h Context.cpp Context.h common.h Swapchain.cpp Swapchain.h Renderer.cpp Renderer.h
        Cache.h objects/RenderPass.h objects/DescriptorOrder.h objects/VkOrder.h objects/RenderPass.cpp
        objects/GraphicsPipeline.cpp objects/GraphicsPipeline.h objects/ShaderModule.cpp objects/ShaderModule.h objects/PipelineLayout.cpp objects/PipelineLayout.h)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw fmt vulkan spdlog glm crampl)
add_dependencies(${PROJECT_NAME} shaders-target)
