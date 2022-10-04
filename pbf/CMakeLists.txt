find_package(Threads REQUIRED)

add_library(pbf_common STATIC
        Cache.h Cache.cpp descriptors/RenderPass.h descriptors/Order.h descriptors/RenderPass.cpp
        descriptors/GraphicsPipeline.cpp descriptors/GraphicsPipeline.h descriptors/ShaderModule.cpp
        descriptors/ShaderModule.h descriptors/PipelineLayout.cpp descriptors/PipelineLayout.h
        descriptors/ComputePipeline.h descriptors/ComputePipeline.cpp
        descriptors/DescriptorSetLayout.h descriptors/DescriptorSetLayout.cpp
        Buffer.cpp Buffer.h MemoryManager.cpp MemoryManager.h RadixSort.cpp RadixSort.h
        NeighbourCellFinder.cpp NeighbourCellFinder.h Selection.cpp Selection.h
        SPH.cpp SPH.h)
target_include_directories(pbf_common PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(pbf_common PUBLIC fmt Vulkan::Vulkan spdlog_header_only glm crampl Threads::Threads)

set(SOURCES main.cpp glfw.h Context.cpp Context.h ContextInterface.h common.h Swapchain.cpp Swapchain.h Renderer.cpp Renderer.h
        Scene.cpp Scene.h Camera.cpp Camera.h IndirectCommandsBuffer.h IndirectCommandsBuffer.cpp Quad.cpp Quad.h
        Simulation.cpp Simulation.h Image.cpp Image.h GUI.cpp GUI.h)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC glfw pbf_common imgui)
add_dependencies(${PROJECT_NAME} shaders-target)
