/**
 *
 *
 * @file Renderer.h
 * @brief 
 * @author clonker
 * @date 9/26/18
 */
#pragma once

#include <pbf/common.h>
#include <pbf/Swapchain.h>

namespace pbf {

class Renderer {
public:

    using StagingFunctor = ClosureContainer;

    explicit Renderer(Context *context);

    void render();

    CacheReference<descriptors::RenderPass> renderPass() const {
        return _renderPass;
    }

    template<typename... Args>
    void stage(Args&&... f) {
        _frameSync[_currentFrameSync].stagingFunctorQueue.emplace_back(std::unique_ptr<StagingFunctor>(new TypedClosureContainer(std::forward<Args>(f)...)));
    }

private:

    void reset();

    Context *_context;
    std::unique_ptr<Swapchain> _swapchain;
    struct FrameSync {
        vk::UniqueSemaphore imageAvailableSemaphore;
        vk::UniqueSemaphore renderFinishedSemaphore;
        vk::UniqueFence fence;
        vk::UniqueCommandBuffer commandBuffer {};
        std::vector<std::unique_ptr<StagingFunctor>> stagingFunctorQueue;

        void reset() {
            commandBuffer.reset();
            stagingFunctorQueue.clear();
        }
    };
    std::vector<FrameSync> _frameSync;
    std::size_t _currentFrameSync = 0;
    std::vector<vk::UniqueCommandBuffer> _commandBuffers;

    vk::UniqueCommandBuffer _stagingCommandBuffer;

    CacheReference<descriptors::RenderPass> _renderPass;
    //CacheReference<descriptors::GraphicsPipeline> _graphicsPipeline;
};

}
