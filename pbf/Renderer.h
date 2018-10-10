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

    CacheReference<objects::RenderPass> renderPass() const {
        return _renderPass;
    }
private:

    void reset();

    Context *_context;
    std::unique_ptr<Swapchain> _swapchain;
    vk::UniqueSemaphore _imageAvailableSemaphore;
    vk::UniqueSemaphore _renderFinishedSemaphore;
    std::vector<vk::UniqueCommandBuffer> _commandBuffers;

    CacheReference<objects::RenderPass> _renderPass;
    CacheReference<objects::GraphicsPipeline> _graphicsPipeline;
};

}
