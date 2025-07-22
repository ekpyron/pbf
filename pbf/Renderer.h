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
struct GlobalAppData;

class Renderer {
public:
    explicit Renderer(InitContext& context, GlobalAppData& globalAppData);

    void render(Scene& scene, GUI& gui, float timestep);

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

	VulkanContext& context() { return _context; }
	[[nodiscard]] VulkanContext const& context() const { return _context; }

    [[nodiscard]] static auto constexpr framePrerenderCount() {
        return 3u;
    }

    [[nodiscard]] std::uint32_t currentFrameSync() const {
        return _currentFrameSync;
    }
private:
    void reset();

    VulkanContext &_context;
    std::unique_ptr<Swapchain> _swapchain;
	vk::UniqueCommandBuffer initCommandBuffer;

    struct FrameSync {
    	vk::UniqueSemaphore imageAvailableSemaphore;
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
