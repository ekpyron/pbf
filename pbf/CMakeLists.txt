find_package(Threads REQUIRED)


set(SOURCES main.cpp glfw.h Context.cpp Context.h common.h Swapchain.cpp Swapchain.h Renderer.cpp Renderer.h
        Cache.h Cache.cpp descriptors/RenderPass.h descriptors/Order.h descriptors/VkOrder.h descriptors/RenderPass.cpp
        descriptors/GraphicsPipeline.cpp descriptors/GraphicsPipeline.h descriptors/ShaderModule.cpp
        descriptors/ShaderModule.h descriptors/PipelineLayout.cpp descriptors/PipelineLayout.h
        descriptors/DescriptorSetLayout.h descriptors/DescriptorSetLayout.cpp
        MemoryManager.cpp MemoryManager.h Buffer.cpp Buffer.h Scene.cpp Scene.h
        Camera.cpp Camera.h IndirectCommandsBuffer.h IndirectCommandsBuffer.cpp Quad.cpp Quad.h)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw fmt vulkan spdlog glm crampl Threads::Threads)
add_dependencies(${PROJECT_NAME} shaders-target)
