/**
 *
 *
 * @file Renderer.cpp
 * @brief 
 * @author clonker
 * @date 9/26/18
 */
#include <pbf/descriptors/GraphicsPipeline.h>
#include "Renderer.h"
#include "Scene.h"
#include <pbf/GUI.h>

static constexpr std::uint64_t TIMEOUT = std::numeric_limits<std::uint64_t>::max();

namespace pbf {

Renderer::Renderer(InitContext &initContext) : _context(initContext.context) {
    _frameSync.reserve(framePrerenderCount());
    for (size_t i = 0; i < framePrerenderCount(); i++) {
        _frameSync.emplace_back(FrameSync{
                _context.device().createSemaphoreUnique({}),
				_context.device().createSemaphoreUnique({}),
                _context.device().createSemaphoreUnique({}),
                _context.device().createFenceUnique(vk::FenceCreateInfo {
					.flags = vk::FenceCreateFlagBits::eSignaled
				})
        });
        PBF_DEBUG_SET_OBJECT_NAME(_context, *_frameSync.back().imageAvailableSemaphore,
                                  fmt::format("Image Available Semaphore #{}", i));
        PBF_DEBUG_SET_OBJECT_NAME(_context, *_frameSync.back().renderFinishedSemaphore,
                                  fmt::format("Render Finished Semaphore #{}", i));
		PBF_DEBUG_SET_OBJECT_NAME(_context, *_frameSync.back().computeFinishedSemaphore,
								  fmt::format("Compute Finished Semaphore #{}", i));
        PBF_DEBUG_SET_OBJECT_NAME(_context, *_frameSync.back().fence, fmt::format("Frame Fence #{}", i));
    }

    {
        _renderPass = _context.cache().fetch(descriptors::RenderPass{
                .attachments = {
                        vk::AttachmentDescription{
                                {},
                                _context.surfaceFormat().format,
                                vk::SampleCountFlagBits::e1,
                                vk::AttachmentLoadOp::eClear,
                                vk::AttachmentStoreOp::eStore,
                                vk::AttachmentLoadOp::eDontCare,
                                vk::AttachmentStoreOp::eDontCare,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::ePresentSrcKHR
                        },
					vk::AttachmentDescription{
						{},
						_context.depthFormat(),
						vk::SampleCountFlagBits::e1,
						vk::AttachmentLoadOp::eClear,
						vk::AttachmentStoreOp::eDontCare,
						vk::AttachmentLoadOp::eDontCare,
						vk::AttachmentStoreOp::eDontCare,
						vk::ImageLayout::eUndefined,
						vk::ImageLayout::eDepthStencilAttachmentOptimal
					}
                },
                .subpasses = {
                        {
                                .flags = {},
                                .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
                                .inputAttachments = {},
                                .colorAttachments = {{0, vk::ImageLayout::eColorAttachmentOptimal}},
                                .resolveAttachments = {},
                                .depthStencilAttachment = {{1, vk::ImageLayout::eDepthStencilAttachmentOptimal}},
                                .preserveAttachments = {}
                        }
                },
                .subpassDependencies = {
                        vk::SubpassDependency{
                                VK_SUBPASS_EXTERNAL,
                                0,
                                vk::PipelineStageFlagBits::eColorAttachmentOutput|vk::PipelineStageFlagBits::eEarlyFragmentTests,
                                vk::PipelineStageFlagBits::eColorAttachmentOutput|vk::PipelineStageFlagBits::eEarlyFragmentTests,
                                {}, vk::AccessFlagBits::eColorAttachmentWrite|vk::AccessFlagBits::eDepthStencilAttachmentWrite, {}
                        }
                },
                PBF_DESC_DEBUG_NAME("Main Renderer RenderPass")
        });
    }
    reset();
}

void Renderer::render(float timestep) {
    const auto &device = _context.device();

    auto &currentFrameSync = _frameSync[_currentFrameSync];

	{
		auto result = device.waitForFences({*currentFrameSync.fence}, static_cast<vk::Bool32>(true), TIMEOUT);
		// TODO: handle result
	}
    currentFrameSync.reset();

//    _renderPass.keepAlive(); --> is kept alive by graphics pipeline (probably)
//    _graphicsPipeline.keepAlive();

    uint32_t imageIndex = 0;
	try {
		auto result = device.acquireNextImageKHR(
			_swapchain->swapchain(), TIMEOUT, *currentFrameSync.imageAvailableSemaphore,
			nullptr, &imageIndex
		);

		switch (result) {
			case vk::Result::eErrorOutOfDateKHR: {
				reset();
				return;
			}
			case vk::Result::eSuccess:
			case vk::Result::eSuboptimalKHR:
				break;
			default: {
				vk::detail::throwResultException(result, "cannot acquire image");
			}
		}
	} catch (vk::OutOfDateKHRError const&) {
		reset();
		return;
	}

    auto buffer = std::move(device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
		.commandPool = _context.commandPool(true),
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1U
    }).front());
    {

#ifndef NDEBUG
        static std::size_t __counter{0};
        PBF_DEBUG_SET_OBJECT_NAME(_context, *buffer, fmt::format("Scene Command Buffer #{}", __counter++));
#endif

        buffer->begin(vk::CommandBufferBeginInfo{
			.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			.pInheritanceInfo = nullptr
		});


        _context.scene().frame(*buffer);

		if (_context.gui().runSPH())
		{
            static size_t numSimulationSteps = 4;
            for(size_t i = 0; i < numSimulationSteps; ++i)
			    _context.scene().simulation().run(*buffer, timestep / float(numSimulationSteps));
		}
        _context.scene().simulation().copy(
                *buffer,
                _context.scene().particleData().buffer(),
                _context.scene().particleData().segmentDeviceSize() * _currentFrameSync
        );

		std::array<vk::ClearValue, 2> clearValues;
        clearValues[0].setColor({std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f}});
		clearValues[1].setDepthStencil(vk::ClearDepthStencilValue{.depth = 1.0f, .stencil = 0});
		buffer->setViewport(0, {
            vk::Viewport{
                0, 0, float(_swapchain->extent().width), float(_swapchain->extent().height), 0.0f, 1.0f
            }
        });
        buffer->setScissor(0, {vk::Rect2D{vk::Offset2D(), _swapchain->extent()}});
        buffer->beginRenderPass(vk::RenderPassBeginInfo{
			.renderPass = *_renderPass,
			.framebuffer = *_swapchain->frameBuffers()[imageIndex],
			.renderArea = vk::Rect2D{{}, _swapchain->extent()},
			.clearValueCount = clearValues.size(),
			.pClearValues = clearValues.data()
        }, vk::SubpassContents::eInline);
        /*buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *_graphicsPipeline);
        buffer->draw(3, 1, 0, 0);*/

		_context.scene().enqueueCommands(*buffer);

		_context.gui().render(*buffer);

        buffer->endRenderPass();
        buffer->end();
    }


    device.resetFences({*currentFrameSync.fence});

	/*
	 currentFrameSync.computeCommandBuffer = std::move(_context->scene().simulation().run());
	{
		vk::PipelineStageFlags waitStages[] = {
			vk::PipelineStageFlagBits::eVertexInput
		};
		_context->graphicsQueue().submit({
											 vk::SubmitInfo{
												 .waitSemaphoreCount = 1,
												 .pWaitSemaphores = &*currentFrameSync.renderFinishedSemaphore,
												 .pWaitDstStageMask = waitStages,
												 .commandBufferCount = 1,
												 .pCommandBuffers = &*currentFrameSync.computeCommandBuffer,
												 .signalSemaphoreCount = 1,
												 .pSignalSemaphores = &*currentFrameSync.computeFinishedSemaphore
											 }
										 });
	}
	 */

	auto const& nextFrameSync = _frameSync.at((_currentFrameSync + 1) % _frameSync.size());
	{
		vk::PipelineStageFlags waitStages[] = {
			vk::PipelineStageFlagBits::eColorAttachmentOutput
		};
		_context.graphicsQueue().submit({vk::SubmitInfo{
			.waitSemaphoreCount = 1u,
			.pWaitSemaphores = &*currentFrameSync.imageAvailableSemaphore,
			.pWaitDstStageMask = waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &*buffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &*currentFrameSync.renderFinishedSemaphore
		}}, *currentFrameSync.fence);
	}

    currentFrameSync.commandBuffer = std::move(buffer);

	try	{
		auto result = _context.presentQueue().presentKHR(vk::PresentInfoKHR{
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &*currentFrameSync.renderFinishedSemaphore,
			.swapchainCount = 1,
			.pSwapchains = &_swapchain->swapchain(),
			.pImageIndices = &imageIndex,
			.pResults = nullptr
		});
		if (result != vk::Result::eSuccess)
			vk::detail::throwResultException(result, "");
	} catch (const vk::OutOfDateKHRError&) {
		reset();
	}

	_currentFrameSync = (_currentFrameSync + 1) % _frameSync.size();
}

void Renderer::reset() {
    auto const &device = _context.device();
    if (_swapchain)
        device.waitIdle();
    _swapchain = std::make_unique<Swapchain>(_context, *_renderPass, _swapchain ? _swapchain->swapchain() : nullptr);
}

}
