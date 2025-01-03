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

class SurfaceReconstruction;

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

    [[nodiscard]] static auto constexpr framePrerenderCount() {
        return 3u;
    }

    [[nodiscard]] std::uint32_t currentFrameSync() const {
        return _currentFrameSync;
    }
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
    	Image depthPingImage;
    	Image depthPongImage;
    	Image thicknessImage;
    	vk::UniqueImageView depthPingView{};
    	vk::UniqueImageView depthPongView{};
    	vk::UniqueImageView thicknessView{};
    	vk::UniqueFramebuffer frameBuffer{};
    };

	OffscreenData& currentOffscreenData()
	{
		return _frameSync.at(currentFrameSync()).offscreenData;
	}
	OffscreenData& offscreenData(size_t _i)
	{
		return _frameSync.at(_i).offscreenData;
	}
private:
    void reset();

    Context &_context;
    std::unique_ptr<Swapchain> _swapchain;
	vk::UniqueCommandBuffer initCommandBuffer;

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

	std::unique_ptr<SurfaceReconstruction> _surfaceReconstruction;

    std::vector<vk::UniqueCommandBuffer> _commandBuffers;

	CacheReference<descriptors::RenderPass> _offscreenRenderPass;
	CacheReference<descriptors::RenderPass> _renderPass;
    //CacheReference<descriptors::GraphicsPipeline> _graphicsPipeline;
};

}
