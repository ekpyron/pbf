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
    explicit Renderer(Context *context);

    void render();

    CacheReference<descriptors::RenderPass> renderPass() const {
        return _renderPass;
    }
private:

    void reset();

    Context *_context;
    std::unique_ptr<Swapchain> _swapchain;
    struct FrameSync {
        vk::UniqueSemaphore imageAvailableSemaphore;
        vk::UniqueSemaphore renderFinishedSemaphore;
        vk::UniqueFence fence;
    };
    std::vector<FrameSync> _frameSync;
    std::size_t _currentFrameSync = 0;
    std::vector<vk::UniqueCommandBuffer> _commandBuffers;

    CacheReference<descriptors::RenderPass> _renderPass;
    CacheReference<descriptors::GraphicsPipeline> _graphicsPipeline;
};

}
