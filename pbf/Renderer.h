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
    explicit Renderer(InitContext& context);

    void render();

    [[nodiscard]] CacheReference<descriptors::RenderPass> renderPass() const {
        return _renderPass;
    }

	template<typename T, typename... Args>
	T& createFrameData(Args&&... args) {
		auto uniqueFrameData = std::make_unique<FrameData<T>>(std::forward<Args>(args)...);
		T& result = uniqueFrameData->data;
		_frameSync[_currentFrameSync].frameData.emplace_back(move(uniqueFrameData));
		return result;
	}

	[[nodiscard]] auto framePrerenderCount() const {
		return 3u;
	}

	[[nodiscard]] std::uint32_t currentFrameSync() const {
		return _currentFrameSync;
	}
	[[nodiscard]] std::uint32_t previousFrameSync() const {
		return (_currentFrameSync + framePrerenderCount() - 1) %  framePrerenderCount();
	}

	Context& context() { return _context; }
	[[nodiscard]] Context const& context() const { return _context; }

private:

    void reset();

    Context &_context;
    std::unique_ptr<Swapchain> _swapchain;
	vk::UniqueCommandBuffer initCommandBuffer;
    struct FrameSync {
        vk::UniqueSemaphore imageAvailableSemaphore;
        vk::UniqueSemaphore renderFinishedSemaphore;
		vk::UniqueSemaphore computeFinishedSemaphore;
        vk::UniqueFence fence;
        vk::UniqueCommandBuffer commandBuffer{};
		vk::UniqueCommandBuffer computeCommandBuffer{};
		std::vector<std::unique_ptr<FrameDataBase>> frameData{};

        void reset() {
            commandBuffer.reset();
			frameData.clear();
        }
    };
    std::vector<FrameSync> _frameSync;
    std::size_t _currentFrameSync = 0;
    std::vector<vk::UniqueCommandBuffer> _commandBuffers;

	CacheReference<descriptors::RenderPass> _renderPass;
    //CacheReference<descriptors::GraphicsPipeline> _graphicsPipeline;
};

}
