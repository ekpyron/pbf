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

	auto framePrerenderCount() const {
		return 3u;
	}

	auto currentFrameSync() const {
		return _currentFrameSync;
	}
	auto previousFrameSync() const {
		return (_currentFrameSync + framePrerenderCount() - 1) %  framePrerenderCount();
	}

private:

    void reset();

    Context *_context;
    std::unique_ptr<Swapchain> _swapchain;
    struct FrameSync {
        vk::UniqueSemaphore imageAvailableSemaphore;
        vk::UniqueSemaphore renderFinishedSemaphore;
		vk::UniqueSemaphore computeFinishedSemaphore;
        vk::UniqueFence fence;
        vk::UniqueCommandBuffer commandBuffer {};
		vk::UniqueCommandBuffer computeCommandBuffer {};
        std::vector<std::unique_ptr<StagingFunctor>> stagingFunctorQueue;

        void reset() {
            commandBuffer.reset();
            stagingFunctorQueue.clear();
        }
    };
    std::vector<FrameSync> _frameSync;
    std::size_t _currentFrameSync = 0;
    std::vector<vk::UniqueCommandBuffer> _commandBuffers;
	bool firstRun = true;

    vk::UniqueCommandBuffer _stagingCommandBuffer;

    CacheReference<descriptors::RenderPass> _renderPass;
    //CacheReference<descriptors::GraphicsPipeline> _graphicsPipeline;
};

}
