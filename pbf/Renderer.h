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

    explicit Renderer(Context &context);

    void render();

    [[nodiscard]] CacheReference<descriptors::RenderPass> renderPass() const {
        return _renderPass;
    }

    template<typename... Args>
    void stage(Args&&... f) {
        _frameSync[_currentFrameSync].stagingFunctorQueue.emplace_back(std::unique_ptr<StagingFunctor>(new TypedClosureContainer(std::forward<Args>(f)...)));
    }

	// TODO: not working! Fix.
	template<typename Data>
	void initUniformBuffer(vk::Buffer _dstBuffer, Data _data) {
		Buffer<Data> srcBuffer{
			_context,
			1,
			vk::BufferUsageFlagBits::eTransferSrc,
			MemoryType::TRANSIENT
		};
		*srcBuffer.data() = _data;
		srcBuffer.flush();
		stage([
			this,
			_dstBuffer,
		    gridDataInitBuffer = std::move(srcBuffer)
		](vk::CommandBuffer buf) {
			buf.pipelineBarrier(
				vk::PipelineStageFlagBits::eHost,
				vk::PipelineStageFlagBits::eTransfer,
				{},
				{
					vk::MemoryBarrier{
						.srcAccessMask = vk::AccessFlagBits::eHostWrite,
						.dstAccessMask = vk::AccessFlagBits::eTransferRead
					}
				},
				{}, {}
			);

			buf.copyBuffer(gridDataInitBuffer.buffer(), _dstBuffer, {
				vk::BufferCopy {
					.srcOffset = 0,
					.dstOffset = 0,
					.size = sizeof(Data)
				}
			});

			buf.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eComputeShader,
				{},
				{
					vk::MemoryBarrier{
						.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
						.dstAccessMask = vk::AccessFlagBits::eUniformRead
					}
				},
				{}, {}
			);
		});
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

	CacheReference<descriptors::RenderPass> _renderPass;
    //CacheReference<descriptors::GraphicsPipeline> _graphicsPipeline;
};

}
