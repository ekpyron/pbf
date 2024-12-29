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

    void render(float timestep);

    [[nodiscard]] CacheReference<descriptors::RenderPass> renderPass() const {
        return _renderPass;
    }
	[[nodiscard]] CacheReference<descriptors::RenderPass> offscreenRenderPass() const {
    	return _offscreenRenderPass;
    }

	template<typename T, typename... Args>
	T& createFrameData(Args&&... args) {
		auto uniqueFrameData = std::make_unique<FrameData<T>>(std::forward<Args>(args)...);
		T& result = uniqueFrameData->data;
		_frameSync[_currentFrameSync].frameData.emplace_back(move(uniqueFrameData));
		return result;
	}

	Context& context() { return _context; }
	[[nodiscard]] Context const& context() const { return _context; }

    [[nodiscard]] auto framePrerenderCount() const {
        return 3u;
    }

    [[nodiscard]] std::uint32_t currentFrameSync() const {
        return _currentFrameSync;
    }

private:
    void reset();

    Context &_context;
    std::unique_ptr<Swapchain> _swapchain;
	vk::UniqueCommandBuffer initCommandBuffer;
	struct OffscreenData
	{
		OffscreenData(InitContext& context, vk::RenderPass renderPass);
		constexpr static vk::Extent2D extent2D()
		{
			return vk::Extent2D{1024, 1024};
		}
		constexpr static vk::Extent3D extent3D()
		{
			return vk::Extent3D{extent2D().width, extent2D().height, 1};
		}
		Image depthImage;
		Image thicknessImage;
		vk::UniqueImageView depthView{};
		vk::UniqueImageView thicknessView{};
		vk::UniqueFramebuffer frameBuffer{};
	};
    struct FrameSync {
    	OffscreenData offscreenData;

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

	CacheReference<descriptors::RenderPass> _offscreenRenderPass;
	CacheReference<descriptors::RenderPass> _renderPass;
    //CacheReference<descriptors::GraphicsPipeline> _graphicsPipeline;
};

}
