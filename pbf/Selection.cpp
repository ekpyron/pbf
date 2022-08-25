#include <pbf/Selection.h>
#include <pbf/descriptors/RenderPass.h>
#include "pbf/descriptors/ShaderModule.h"
#include "Scene.h"
#include "Renderer.h"

namespace pbf {

Selection::Selection(InitContext& initContext):
_context(initContext.context),
_depthBuffer(initContext.context, initContext.context.depthFormat(), vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::Extent3D{.width = 1, .height = 1, .depth = 1}),
_selectionImage(initContext.context, vk::Format::eR32Uint, vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eTransferSrc, vk::Extent3D{.width = 1, .height = 1, .depth = 1}),
_selectionBuffer(initContext.context, 1, vk::BufferUsageFlagBits::eTransferDst, MemoryType::DYNAMIC),
_indexBuffer(initContext.context, 6, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, pbf::MemoryType::STATIC)
{
	_renderPass = _context.cache().fetch(descriptors::RenderPass{
		.attachments = {
			vk::AttachmentDescription{
				{},
				vk::Format::eR32Uint,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eTransferSrcOptimal,
				vk::ImageLayout::eColorAttachmentOptimal
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
		PBF_DESC_DEBUG_NAME("Selection Renderer RenderPass")
	});

	_selectionImageView = _context.device().createImageViewUnique(vk::ImageViewCreateInfo{
		.image = _selectionImage.image(),
		.viewType = vk::ImageViewType::e2D,
		.format = vk::Format::eR32Uint,
		.subresourceRange = vk::ImageSubresourceRange{
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	});
	_depthBufferView = _context.device().createImageViewUnique(vk::ImageViewCreateInfo{
		.image = _depthBuffer.image(),
		.viewType = vk::ImageViewType::e2D,
		.format = _context.depthFormat(),
		.subresourceRange = vk::ImageSubresourceRange{
			.aspectMask = vk::ImageAspectFlagBits::eDepth,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	});

	{
		std::array<vk::ImageView, 2> ivs = {
			*_selectionImageView,
			*_depthBufferView
		};

		_framebuffer = _context.device().createFramebufferUnique(
			vk::FramebufferCreateInfo{
				.renderPass = *_renderPass,
				.attachmentCount = ivs.size(),
				.pAttachments = ivs.data(),
				.width = 1,
				.height = 1,
				.layers = 1,
			}
		);
	}
	{
		_graphicsPipeline = _context.cache().fetch(
			pbf::descriptors::GraphicsPipeline{
				.shaderStages = {
					{
						.stage = vk::ShaderStageFlagBits::eVertex,
						.module = _context.cache().fetch(
							descriptors::ShaderModule{
								.source = descriptors::ShaderModule::File{"shaders/selection/particle.vert.spv"},
								PBF_DESC_DEBUG_NAME("shaders/selection/particle.vert.spv Vertex Shader")
							}),
						.entryPoint = "main"
					},
					{
						.stage = vk::ShaderStageFlagBits::eFragment,
						.module = _context.cache().fetch(
							descriptors::ShaderModule{
								.source = descriptors::ShaderModule::File{"shaders/selection/particle.frag.spv"},
								PBF_DESC_DEBUG_NAME("shaders/selection/particle.frag.spv Fragment Shader")
							}),
						.entryPoint = "main"
					}
				},
				.vertexBindingDescriptions = {
					vk::VertexInputBindingDescription{
						.binding = 0,
						.stride = sizeof(ParticleData),
						.inputRate = vk::VertexInputRate::eInstance
					}
				},
				.vertexInputAttributeDescriptions = {
					vk::VertexInputAttributeDescription{
						.location = 0,
						.binding = 0,
						.format = vk::Format::eR32G32B32Sfloat,
						.offset = offsetof(ParticleData, position)
					}
				},
				.primitiveTopology = vk::PrimitiveTopology::eTriangleList,
				.rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo().setLineWidth(1.0f),
				.depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo{
					.depthTestEnable = true,
					.depthWriteEnable = true,
					.depthCompareOp = vk::CompareOp::eLess,
					.depthBoundsTestEnable = false,
					.stencilTestEnable = false,
					.minDepthBounds = 0.0f,
					.maxDepthBounds = 1.0f
				},
				.colorBlendAttachmentStates = {
					vk::PipelineColorBlendAttachmentState().setColorWriteMask(
						vk::ColorComponentFlagBits::eR
					)
				},
				.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
				.pipelineLayout = _context.cache().fetch(
					pbf::descriptors::PipelineLayout{
						.setLayouts = {{
							_context.globalDescriptorSetLayout()
						}},
						PBF_DESC_DEBUG_NAME("Selection Renderer Pipeline Layout")
					}),
				.renderPass = _renderPass,
				PBF_DESC_DEBUG_NAME("Selection Renderer Graphics Pipeline")
			});
	}


	auto& initBuffer = initContext.createInitData<pbf::Buffer<std::byte>>(
		_context, sizeof(std::uint16_t) * 6, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
	);

	std::uint16_t *indices = reinterpret_cast<std::uint16_t*>(reinterpret_cast<intptr_t>(initBuffer.data()));
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 0;
	indices[4] = 2;
	indices[5] = 3;
	initBuffer.flush();
	auto& initCmdBuf = initContext.initCommandBuffer;
	initCmdBuf->copyBuffer(initBuffer.buffer(), _indexBuffer.buffer(), {
		vk::BufferCopy {
			0, 0, _indexBuffer.deviceSize()
		}
	});
	initCmdBuf->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexInput, {}, {}, {vk::BufferMemoryBarrier{
		.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask = vk::AccessFlagBits::eIndexRead,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.buffer = _indexBuffer.buffer(),
		.offset = 0,
		.size = _indexBuffer.deviceSize()
	}}, {});
	initCmdBuf->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, {
									vk::ImageMemoryBarrier{
										.srcAccessMask = vk::AccessFlagBits::eNone,
										.dstAccessMask = vk::AccessFlagBits::eTransferRead,
										.oldLayout = vk::ImageLayout::eUndefined,
										.newLayout = vk::ImageLayout::eTransferSrcOptimal,
										.srcQueueFamilyIndex = _context.graphicsQueueFamily(),
										.dstQueueFamilyIndex = _context.graphicsQueueFamily(),
										.image = _selectionImage.image(),
										.subresourceRange = vk::ImageSubresourceRange{
											.aspectMask = vk::ImageAspectFlagBits::eColor,
											.baseMipLevel = 0,
											.levelCount = 1,
											.baseArrayLayer = 0,
											.layerCount = 1
										}
									}
								});


}

uint32_t Selection::operator()(RingBuffer<ParticleData>& particleData, uint32_t x, uint32_t y) {
	vk::UniqueCommandBuffer cmdBuf = std::move(_context.device().allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
		.commandPool = _context.commandPool(true),
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1
	}).front());

	cmdBuf->begin(vk::CommandBufferBeginInfo{
		.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
		.pInheritanceInfo = nullptr
	});

	std::array<vk::ClearValue, 2> clearValues{
		vk::ClearValue{vk::ClearColorValue{}.setUint32({-1u,0,0,0})},
		vk::ClearValue{vk::ClearDepthStencilValue{}.setDepth(1.0f).setStencil(0)}
	};
	auto [width, height] = _context.window().framebufferSize();
	cmdBuf->setViewport(0, {
		vk::Viewport{
			.x = -float(x),
			.y = -float(y),
			.width = float(width),
			.height = float(height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		}
	});
	cmdBuf->setScissor(0, {vk::Rect2D{vk::Offset2D(), vk::Extent2D{.width = 1, .height = 1}}});

	cmdBuf->beginRenderPass(vk::RenderPassBeginInfo{
		.renderPass = *_renderPass,
		.framebuffer = *_framebuffer,
		.renderArea = vk::Rect2D{
			.offset = vk::Offset2D{
				.x = 0,
				.y = 0
			},
			.extent = vk::Extent2D{
				.width = 1,
				.height = 1
			}
		},
		.clearValueCount = clearValues.size(),
		.pClearValues = clearValues.data()
	}, vk::SubpassContents::eInline);

	cmdBuf->bindPipeline(vk::PipelineBindPoint::eGraphics, *_graphicsPipeline);
	cmdBuf->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *(_graphicsPipeline.descriptor().pipelineLayout), 0, { _context.globalDescriptorSet()}, {});
	cmdBuf->bindIndexBuffer(_indexBuffer.buffer(), 0, vk::IndexType::eUint16);
	cmdBuf->bindVertexBuffers(0, {particleData.buffer()}, {particleData.segment(_context.renderer().currentFrameSync()).offset});

	cmdBuf->drawIndexed(6, particleData.size(), 0, 0, 0);

	cmdBuf->endRenderPass();

	cmdBuf->pipelineBarrier(
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eTransfer,
		{},
		{},
		{},
		{
			vk::ImageMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
				.dstAccessMask = vk::AccessFlagBits::eTransferRead,
				.oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
				.newLayout = vk::ImageLayout::eTransferSrcOptimal,
				.srcQueueFamilyIndex = _context.graphicsQueueFamily(),
				.dstQueueFamilyIndex = _context.graphicsQueueFamily(),
				.image = _selectionImage.image(),
				.subresourceRange = vk::ImageSubresourceRange{
					.aspectMask = vk::ImageAspectFlagBits::eColor,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			}
		}
	);

	cmdBuf->copyImageToBuffer(_selectionImage.image(), vk::ImageLayout::eTransferSrcOptimal, _selectionBuffer.buffer(),{
		vk::BufferImageCopy{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = vk::ImageSubresourceLayers{
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.imageOffset = vk::Offset3D{0,0,0},
			.imageExtent = vk::Extent3D{1,1,1}
		}
	});

	cmdBuf->end();

	_context.graphicsQueue().submit({
		vk::SubmitInfo{
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &*cmdBuf,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr
		}
	});
	_context.graphicsQueue().waitIdle();

	return *_selectionBuffer.data();
}

}