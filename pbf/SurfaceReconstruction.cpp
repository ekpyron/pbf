//
// Created by daniel on 12/29/24.
//

#include "SurfaceReconstruction.h"

#include "VulkanContext.h"
#include "Renderer.h"
#include <list>

#include "App.h"

namespace pbf
{

SurfaceReconstruction::FrameData::FrameData(InitContext& _initContext, Renderer& _renderer, GlobalAppData& globalAppData, SurfaceReconstruction& _parent):
	depthPingImage(
		_initContext.context,
		vk::Format::eD32Sfloat,
		vk::ImageUsageFlagBits::eDepthStencilAttachment|vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eInputAttachment|vk::ImageUsageFlagBits::eTransferSrc,
		extent3D()
	),
	depthPongImage(
		_initContext.context,
		vk::Format::eD32Sfloat,
		vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eInputAttachment|vk::ImageUsageFlagBits::eTransferSrc|vk::ImageUsageFlagBits::eSampled,
		extent3D()
	),
	thicknessImage(
	_initContext.context,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eTransferSrc|vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eInputAttachment, // TODO: remove transfer src
		extent3D()
	)

{
	VulkanContext& context = _initContext.context;
	depthPingView = context.device().createImageViewUnique(vk::ImageViewCreateInfo{
		.flags = {},
		.image = depthPingImage.image(),
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
	depthPongView = context.device().createImageViewUnique(vk::ImageViewCreateInfo{
		.flags = {},
		.image = depthPongImage.image(),
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
	thicknessView = context.device().createImageViewUnique(vk::ImageViewCreateInfo{
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
		*depthPingView
	};
	spdlog::get("console")->debug("Create Surface Reconstruction FrameData Framebuffer");

	frameBuffer = context.device().createFramebufferUnique(vk::FramebufferCreateInfo{
		.flags = {},
		.renderPass = *_renderer.offscreenRenderPass(),
		.attachmentCount = attachments.size(),
		.pAttachments = attachments.data(),
		.width = extent2D().width,
		.height = extent2D().height,
		.layers = 1,
	});

	spdlog::get("console")->debug("Created Surface Reconstruction FrameData Framebuffer");

	_initContext.initCommandBuffer->pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eComputeShader, {}, {}, {}, {
			vk::ImageMemoryBarrier{
				.srcAccessMask = {},
				.dstAccessMask = {},
				.oldLayout = vk::ImageLayout::eUndefined,
				.newLayout = vk::ImageLayout::eGeneral,
				.image = depthPongImage.image(),
				.subresourceRange = {
					.aspectMask = vk::ImageAspectFlagBits::eDepth,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			}
		});


	{
		std::vector<vk::DescriptorSetLayout> setLayouts;
		_parent._depthBlurPipeline.keepAlive();
		_parent._reconstructNormalsPipeline.keepAlive();
		for (auto& layout: _parent._depthBlurPipeline->pipelineLayout.descriptor().setLayouts)
			setLayouts.emplace_back(*layout);
		for (auto& layout: _parent._depthBlurPipeline->pipelineLayout.descriptor().setLayouts)
			setLayouts.emplace_back(*layout);
		for (auto& layout: _parent._reconstructNormalsPipeline->pipelineLayout.descriptor().setLayouts)
			setLayouts.emplace_back(*layout);
		{
			auto allocatedDescriptorSets = context.device().allocateDescriptorSetsUnique(
				vk::DescriptorSetAllocateInfo{
					.descriptorPool = context.descriptorPool(),
					.descriptorSetCount = static_cast<uint32_t>(setLayouts.size()),
					.pSetLayouts = setLayouts.data()
				}
			);
			depthBlurDescriptorSets.inputSampler = std::move(allocatedDescriptorSets.at(0));
			depthBlurDescriptorSets.outputStorageImage = std::move(allocatedDescriptorSets.at(1));
			//depthBlurDescriptorSets.blurDirUniformBuffer = std::move(allocatedDescriptorSets.at(2));
			depthBlurPongDescriptorSets.inputSampler = std::move(allocatedDescriptorSets.at(2));
			depthBlurPongDescriptorSets.outputStorageImage = std::move(allocatedDescriptorSets.at(3));
			//depthBlurPongDescriptorSets.blurDirUniformBuffer = std::move(allocatedDescriptorSets.at(5));
			reconstructNormalDescriptorSets.inputSampler = std::move(allocatedDescriptorSets.at(4));
			reconstructNormalDescriptorSets.outputStorageImage = std::move(allocatedDescriptorSets.at(5));
			//reconstructNormalDescriptorSets.uniformBuffer = std::move(allocatedDescriptorSets.at(6));
			// TODO: note: we're allocating one more unused descriptor set here currently.
		}


		std::vector<vk::WriteDescriptorSet> descriptorWrites;
	    auto blurDirBufferInfo = _parent.blurDirBuffer.fullBufferInfo();
	    std::list<vk::DescriptorImageInfo> imageInfos;
	    vk::DescriptorImageInfo& inputImageInfo = imageInfos.emplace_back(vk::DescriptorImageInfo{
	        .sampler = *_parent.depthSampler,
	        .imageView = *depthPingView,
	        .imageLayout = vk::ImageLayout::eGeneral // TODO: choose optimal layout
	    });
	    vk::DescriptorImageInfo& outputImageInfo = imageInfos.emplace_back(vk::DescriptorImageInfo{
	        .sampler = *_parent.depthSampler,
	        .imageView = *depthPongView,
	        .imageLayout = vk::ImageLayout::eGeneral
	    });
		vk::DescriptorImageInfo& thicknessImageInfo = imageInfos.emplace_back(vk::DescriptorImageInfo{
			.imageView = *thicknessView,
			.imageLayout = vk::ImageLayout::eGeneral
		});
		// depth blur descriptor writes
		{
			descriptorWrites.emplace_back(
				vk::WriteDescriptorSet{
					.dstSet = *depthBlurDescriptorSets.inputSampler,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eCombinedImageSampler,
					.pImageInfo = &inputImageInfo,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				}
			);
			descriptorWrites.emplace_back(
				vk::WriteDescriptorSet{
					.dstSet = *depthBlurDescriptorSets.outputStorageImage,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eStorageImage,
					.pImageInfo = &outputImageInfo,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				}
			);
			/*descriptorWrites.emplace_back(
				vk::WriteDescriptorSet{
					.dstSet = *depthBlurDescriptorSets.blurDirUniformBuffer,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eUniformBuffer,
					.pImageInfo = nullptr,
					.pBufferInfo = &blurDirBufferInfo,
					.pTexelBufferView = nullptr
				}
			);*/
		}
		// depth blur pong descriptor writes
		{
			descriptorWrites.emplace_back(
				vk::WriteDescriptorSet{
					.dstSet = *depthBlurPongDescriptorSets.inputSampler,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eCombinedImageSampler,
					.pImageInfo = &outputImageInfo,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				}
			);
			descriptorWrites.emplace_back(
				vk::WriteDescriptorSet{
					.dstSet = *depthBlurPongDescriptorSets.outputStorageImage,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eStorageImage,
					.pImageInfo = &inputImageInfo,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				}
			);
			/*descriptorWrites.emplace_back(
				vk::WriteDescriptorSet{
					.dstSet = *depthBlurPongDescriptorSets.blurDirUniformBuffer,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eUniformBuffer,
					.pImageInfo = nullptr,
					.pBufferInfo = &blurDirBufferInfo,
					.pTexelBufferView = nullptr
				}
			);*/
		}		// reconstruct normal descriptor writes
		{
			descriptorWrites.emplace_back(
				vk::WriteDescriptorSet{
					.dstSet = *reconstructNormalDescriptorSets.inputSampler,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eCombinedImageSampler,
					.pImageInfo = &outputImageInfo,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				}
			);
			descriptorWrites.emplace_back(
				vk::WriteDescriptorSet{
					.dstSet = *reconstructNormalDescriptorSets.outputStorageImage,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eStorageImage,
					.pImageInfo = &thicknessImageInfo,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				}
			);
			/*
			descriptorWrites.emplace_back(
				vk::WriteDescriptorSet{
					.dstSet = *reconstructNormalDescriptorSets.uniformBuffer,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eUniformBuffer,
					.pImageInfo = nullptr,
					.pBufferInfo = &blurDirBufferInfo,
					.pTexelBufferView = nullptr
				}
			);*/
			reconstructNormalDescriptorSets.uniformBuffer = globalAppData.globalDescriptorSet();

		}
	    context.device().updateDescriptorSets(descriptorWrites, {});
	}

}

SurfaceReconstruction::SurfaceReconstruction(InitContext& _initContext, Renderer& _renderer, GlobalAppData& globalAppData):
context(_initContext.context),
frameSyncData(_renderer)
{
    blurDirBuffer = Buffer<BlurDir>(context, 1, vk::BufferUsageFlagBits::eUniformBuffer, MemoryType::STATIC);
    Cache& cache = context.cache();

    depthSampler = context.device().createSamplerUnique(
        vk::SamplerCreateInfo{
            .magFilter = vk::Filter::eNearest,
            .minFilter = vk::Filter::eNearest,
            .mipmapMode = vk::SamplerMipmapMode::eNearest,
            .addressModeU = vk::SamplerAddressMode::eClampToEdge,
            .addressModeV = vk::SamplerAddressMode::eClampToEdge,
            .addressModeW = vk::SamplerAddressMode::eClampToEdge,
            .mipLodBias = 0.0f,
            .anisotropyEnable = false,
            .maxAnisotropy = {},
            .compareEnable = false,
            .compareOp = vk::CompareOp::eNever,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = {},
            .unnormalizedCoordinates = true,
        }
    );
    {
        auto depthInputSetLayout = cache.fetch(descriptors::DescriptorSetLayout{
            .createFlags = {},
            .bindings = {
                {
                    .binding = 0,
                    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                    .descriptorCount = 1,
                    .stageFlags = vk::ShaderStageFlagBits::eCompute
                }
            },
            PBF_DESC_DEBUG_NAME("SurfaceReconstruction: depth input set layout")
        });
        descriptorSetCacheReferences.emplace_back(depthInputSetLayout);
        auto depthOutputSetLayout = cache.fetch(descriptors::DescriptorSetLayout{
            .createFlags = {},
            .bindings = {
                {
                    .binding = 0,
                    .descriptorType = vk::DescriptorType::eStorageImage,
                    .descriptorCount = 1,
                    .stageFlags = vk::ShaderStageFlagBits::eCompute
                }
            },
            PBF_DESC_DEBUG_NAME("SurfaceReconstruction: depth output set layout")
        });
        descriptorSetCacheReferences.emplace_back(depthOutputSetLayout);
        auto blurredDepthOutputSetLayout = cache.fetch(descriptors::DescriptorSetLayout{
            .createFlags = {},
            .bindings = {
                {
                    .binding = 0,
                    .descriptorType = vk::DescriptorType::eStorageImage,
                    .descriptorCount = 1,
                    .stageFlags = vk::ShaderStageFlagBits::eCompute
                }
            },
            PBF_DESC_DEBUG_NAME("SurfaceReconstruction: blurred depth output set layout")
        });
        descriptorSetCacheReferences.emplace_back(blurredDepthOutputSetLayout);
        auto blurDirection = cache.fetch(descriptors::DescriptorSetLayout{
            .createFlags = {},
            .bindings = {
                {
                    .binding = 0,
                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                    .descriptorCount = 1,
                    .stageFlags = vk::ShaderStageFlagBits::eCompute
                }
            },
            PBF_DESC_DEBUG_NAME("SurfaceReconstruction: blur direction set layout")
        });
        descriptorSetCacheReferences.emplace_back(blurDirection);

        _depthBlurPipeline = cache.fetch(
            descriptors::ComputePipeline{
                .flags = {},
                .shaderStage = descriptors::ShaderStage {
                    .module = cache.fetch(
                    descriptors::ShaderModule{
                        .source = descriptors::ShaderModule::File{"shaders/surface/depthblur.comp.spv"},
                        PBF_DESC_DEBUG_NAME("SurfaceReconstruction: depth blur shader module")
                    }),
                    .specialization = {}
                },
                PBF_DESC_DEBUG_NAME("SurfaceReconstruction: depth blur shader pipeline")
            }
        );

    	_reconstructNormalsPipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/surface/reconstruct_normals.comp.spv"},
						PBF_DESC_DEBUG_NAME("SurfaceReconstruction: reconstruct normals shader module")
					}),
					.specialization = {}
				},
				PBF_DESC_DEBUG_NAME("SurfaceReconstruction: reconstruct normals shader pipeline")
			}
		);
    }

    frameSyncData.create(_initContext, _renderer, globalAppData, *this);
}

void SurfaceReconstruction::run(vk::CommandBuffer& _buf)
{
    for (auto& cacheRef: descriptorSetCacheReferences)
        cacheRef.keepAlive();

	auto& frameData = frameSyncData.getCurrent();

    _buf.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eComputeShader, {}, {}, {}, {
            vk::ImageMemoryBarrier{
                .srcAccessMask = {},
                .dstAccessMask = {},
                .oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
                .newLayout = vk::ImageLayout::eGeneral,
                .image = frameData.depthPingImage.image(),
                .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eDepth,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            }
        });

	for (int i = 0; i < 8; i++)
	{

    _buf.bindPipeline(vk::PipelineBindPoint::eCompute, *_depthBlurPipeline->pipeline);

    _buf.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        *_depthBlurPipeline->pipelineLayout,
        0,
        frameData.depthBlurDescriptorSets.all(),
        {}
    );
    _buf.dispatch(1024 / 256,  1024, 1);

	_buf.pipelineBarrier(
	vk::PipelineStageFlagBits::eComputeShader,
	vk::PipelineStageFlagBits::eComputeShader, {}, {}, {}, {
		vk::ImageMemoryBarrier{
			.srcAccessMask = {},
			.dstAccessMask = {},
			.oldLayout = vk::ImageLayout::eGeneral,
			.newLayout = vk::ImageLayout::eGeneral,
			.image = frameData.depthPongImage.image(),
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eDepth,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		}
	});

	_buf.bindDescriptorSets(
	vk::PipelineBindPoint::eCompute,
	*_depthBlurPipeline->pipelineLayout,
	0,
	frameData.depthBlurPongDescriptorSets.all(),
	{}
);
	_buf.dispatch(1024 / 256,  1024, 1);

	_buf.pipelineBarrier(
	vk::PipelineStageFlagBits::eComputeShader,
	vk::PipelineStageFlagBits::eComputeShader, {}, {}, {}, {
		vk::ImageMemoryBarrier{
			.srcAccessMask = {},
			.dstAccessMask = {},
			.oldLayout = vk::ImageLayout::eGeneral,
			.newLayout = vk::ImageLayout::eGeneral,
			.image = frameData.depthPingImage.image(),
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eDepth,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		}
	});

	_buf.bindDescriptorSets(
		vk::PipelineBindPoint::eCompute,
		*_depthBlurPipeline->pipelineLayout,
		0,
		frameData.depthBlurDescriptorSets.all(),
		{}
	);
	_buf.dispatch(1024 / 256,  1024, 1);

	_buf.pipelineBarrier(
	vk::PipelineStageFlagBits::eComputeShader,
	vk::PipelineStageFlagBits::eComputeShader, {}, {}, {}, {
		vk::ImageMemoryBarrier{
			.srcAccessMask = {},
			.dstAccessMask = {},
			.oldLayout = vk::ImageLayout::eGeneral,
			.newLayout = vk::ImageLayout::eGeneral,
			.image = frameData.depthPongImage.image(),
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eDepth,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		}
	});
	}



	_buf.bindPipeline(vk::PipelineBindPoint::eCompute, *_reconstructNormalsPipeline->pipeline);

	_buf.bindDescriptorSets(
		vk::PipelineBindPoint::eCompute,
		*_reconstructNormalsPipeline->pipelineLayout,
		0,
		frameData.reconstructNormalDescriptorSets.all(),
		{}
	);
	_buf.dispatch(1024 / 256,  1024, 1);

	_buf.pipelineBarrier(
vk::PipelineStageFlagBits::eComputeShader,
vk::PipelineStageFlagBits::eComputeShader, {}, {}, {}, {
	vk::ImageMemoryBarrier{
		.srcAccessMask = {},
		.dstAccessMask = {},
		.oldLayout = vk::ImageLayout::eGeneral,
		.newLayout = vk::ImageLayout::eGeneral,
		.image = frameData.thicknessImage.image(),
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	}
});

}


}
