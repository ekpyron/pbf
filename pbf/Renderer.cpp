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

Renderer::OffscreenData::OffscreenData(InitContext& _context, vk::RenderPass _renderPass):
	depthImage(
		_context.context,
		vk::Format::eD32Sfloat,
		vk::ImageUsageFlagBits::eDepthStencilAttachment|vk::ImageUsageFlagBits::eInputAttachment,
		extent3D()
	),
	thicknessImage(
	_context.context,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eInputAttachment|vk::ImageUsageFlagBits::eTransferSrc, // TODO: remove transfer src
		extent3D()
	)

{
	depthView = _context.context.device().createImageViewUnique(vk::ImageViewCreateInfo{
		.flags = {},
		.image = depthImage.image(),
		.viewType = vk::ImageViewType::e2D,
		.format = vk::Format::eD32Sfloat,
		.components = vk::ComponentMapping{},
		.subresourceRange = vk::ImageSubresourceRange{
			.aspectMask = vk::ImageAspectFlagBits::eDepth,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		}
	});
	thicknessView = _context.context.device().createImageViewUnique(vk::ImageViewCreateInfo{
		.flags = {},
		.image = thicknessImage.image(),
		.viewType = vk::ImageViewType::e2D,
		.format = vk::Format::eR8G8B8A8Unorm,
		.components = vk::ComponentMapping{},
		.subresourceRange = vk::ImageSubresourceRange{
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		}
	});
	std::array attachments = {
		*thicknessView,
		*depthView
	};
	frameBuffer = _context.context.device().createFramebufferUnique(vk::FramebufferCreateInfo{
		.flags = {},
		.renderPass = _renderPass,
		.attachmentCount = attachments.size(),
		.pAttachments = attachments.data(),
		.width = extent2D().width,
		.height = extent2D().height,
		.layers = 1,
	});


}


Renderer::Renderer(InitContext &initContext) : _context(initContext.context) {
    {
        _offscreenRenderPass = _context.cache().fetch(descriptors::RenderPass{
                .attachments = {
                        vk::AttachmentDescription{
                                {},
                                vk::Format::eR8G8B8A8Unorm,
                                vk::SampleCountFlagBits::e1,
                                vk::AttachmentLoadOp::eClear,
                                vk::AttachmentStoreOp::eStore,
                                vk::AttachmentLoadOp::eDontCare,
                                vk::AttachmentStoreOp::eDontCare,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eTransferSrcOptimal
                        },
					vk::AttachmentDescription{
						{},
						vk::Format::eD32Sfloat,
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
                PBF_DESC_DEBUG_NAME("Offscreen Renderer RenderPass")
        });
    }

	_frameSync.reserve(framePrerenderCount());
    for (size_t i = 0; i < framePrerenderCount(); i++) {
        _frameSync.emplace_back(FrameSync{
			OffscreenData{initContext, *_offscreenRenderPass},
			_context.device().createSemaphoreUnique({}),
			_context.device().createSemaphoreUnique({}),
			_context.device().createSemaphoreUnique({}),
			_context.device().createFenceUnique(vk::FenceCreateInfo {
				.flags = vk::FenceCreateFlagBits::eSignaled
			}),
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
                                vk::AttachmentLoadOp::eLoad,
                                vk::AttachmentStoreOp::eStore,
                                vk::AttachmentLoadOp::eDontCare,
                                vk::AttachmentStoreOp::eDontCare,
                                vk::ImageLayout::eTransferDstOptimal,
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
				0, 0, float(currentFrameSync.offscreenData.extent2D().width), float(currentFrameSync.offscreenData.extent2D().height), 0.0f, 1.0f
			}
		});
    	buffer->setScissor(0, {vk::Rect2D{vk::Offset2D(), currentFrameSync.offscreenData.extent2D()}});
    	buffer->beginRenderPass(vk::RenderPassBeginInfo{
			.renderPass = *_offscreenRenderPass,
			.framebuffer = *currentFrameSync.offscreenData.frameBuffer,
			.renderArea = vk::Rect2D{{},currentFrameSync.offscreenData.extent2D()},
			.clearValueCount = clearValues.size(),
			.pClearValues = clearValues.data()
		}, vk::SubpassContents::eInline);

    	_context.scene().enqueueCommands(*buffer);

    	buffer->endRenderPass();

    	vk::ImageBlit blit{
    		.srcSubresource = {
    			.aspectMask = vk::ImageAspectFlagBits::eColor,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.srcOffsets = std::array{vk::Offset3D{
				.x = 0, .y = 0, .z = 0
			}, vk::Offset3D{
				currentFrameSync.offscreenData.extent2D().width,
				currentFrameSync.offscreenData.extent2D().height,
				1
			}},
			.dstSubresource = {
    			.aspectMask = vk::ImageAspectFlagBits::eColor,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.dstOffsets = std::array{vk::Offset3D{
				.x = 0, .y = 0, .z = 0
			}, vk::Offset3D{
				_swapchain->extent().width,
				_swapchain->extent().height,
				1
			}}
    	};


    	buffer->pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eComputeShader, {}, {}, {}, {
				vk::ImageMemoryBarrier{
					.srcAccessMask = {},
					.dstAccessMask = {},
					.oldLayout = vk::ImageLayout::eUndefined,
					.newLayout = vk::ImageLayout::eTransferDstOptimal,
					.image = _swapchain->images()[imageIndex],
					.subresourceRange = {
						.aspectMask = vk::ImageAspectFlagBits::eColor,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1
					}
				}
			});

    	buffer->blitImage(
			currentFrameSync.offscreenData.thicknessImage.image(),
			vk::ImageLayout::eTransferSrcOptimal,
			_swapchain->images()[imageIndex],
			vk::ImageLayout::eTransferDstOptimal,
			1,
			&blit,
			vk::Filter::eLinear
		);

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
