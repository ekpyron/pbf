//
// Created by daniel on 12/29/24.
//

#include "SurfaceReconstruction.h"

#include "Context.h"
#include "Renderer.h"
#include "descriptors/DescriptorSet.h"
#include <list>

namespace pbf
{

SurfaceReconstruction::SurfaceReconstruction(Context& _context): context(_context)
{
    blurDirBuffer = Buffer<BlurDir>(context, 1, vk::BufferUsageFlagBits::eUniformBuffer, MemoryType::STATIC);
    Cache& cache = context.cache();

    depthSampler = context.device().createSamplerUnique(
        vk::SamplerCreateInfo{
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
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

        auto depthBlurPipelineLayout = cache.fetch(
                descriptors::PipelineLayout{
                    .setLayouts = {depthInputSetLayout, blurredDepthOutputSetLayout, blurDirection},
                    .pushConstants = {},
                    PBF_DESC_DEBUG_NAME("Depth blur pipeline layout")
                });
        _depthBlurPipeline = cache.fetch(
            descriptors::ComputePipeline{
                .flags = {},
                .shaderStage = descriptors::ShaderStage {
                    .stage = vk::ShaderStageFlagBits::eCompute,
                    .module = cache.fetch(
                    descriptors::ShaderModule{
                        .source = descriptors::ShaderModule::File{"shaders/surface/depthblur.comp.spv"},
                        PBF_DESC_DEBUG_NAME("SurfaceReconstruction: depth blur shader module")
                    }),
                    .entryPoint = "main",
                    .specialization = {}
                },
                .pipelineLayout = depthBlurPipelineLayout,
                PBF_DESC_DEBUG_NAME("SurfaceReconstruction: depth blur shader pipeline")
            }
        );

        std::vector<vk::DescriptorSetLayout> setLayouts;
        for (size_t i = 0; i < context.renderer().framePrerenderCount(); ++i)
            for (auto const& set: depthBlurPipelineLayout.descriptor().setLayouts)
                setLayouts.emplace_back(*set);
        _descriptorSets = context.device().allocateDescriptorSetsUnique(
            vk::DescriptorSetAllocateInfo{
                .descriptorPool = context.descriptorPool(),
                .descriptorSetCount = 3 * context.renderer().framePrerenderCount(),
                .pSetLayouts = setLayouts.data()
            }
        );
    }

    initDescriptorSets();
}

void SurfaceReconstruction::initDescriptorSets()
{
    std::vector<vk::WriteDescriptorSet> descriptorWrites;
    size_t prerenderCount = context.renderer().framePrerenderCount();
    auto blurDirBufferInfo = blurDirBuffer.fullBufferInfo();
    std::list<vk::DescriptorImageInfo> imageInfos;
    for (size_t frameSyncI = 0; frameSyncI < prerenderCount; ++frameSyncI)
    {
        vk::DescriptorImageInfo& inputImageInfo = imageInfos.emplace_back(vk::DescriptorImageInfo{
            .sampler = *depthSampler,
            .imageView = *context.renderer().offscreenData(frameSyncI).depthPingView,
            .imageLayout = vk::ImageLayout::eGeneral // TODO: choose optimal layout
        });
        vk::DescriptorImageInfo& outputImageInfo = imageInfos.emplace_back(vk::DescriptorImageInfo{
            .sampler = *depthSampler,
            .imageView = *context.renderer().offscreenData(frameSyncI).depthPongView,
            .imageLayout = vk::ImageLayout::eGeneral
        });
        descriptorWrites.emplace_back(
            vk::WriteDescriptorSet{
                .dstSet = *_descriptorSets.at(3 * frameSyncI + 0),
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
                .dstSet = *_descriptorSets.at(3 * frameSyncI + 1),
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageImage,
                .pImageInfo = &outputImageInfo,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr
            }
        );
        descriptorWrites.emplace_back(
            vk::WriteDescriptorSet{
                .dstSet = *_descriptorSets.at(3 * frameSyncI + 2),
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pImageInfo = nullptr,
                .pBufferInfo = &blurDirBufferInfo,
                .pTexelBufferView = nullptr
            }
        );
    }
    context.device().updateDescriptorSets(descriptorWrites, {});
}

void SurfaceReconstruction::run(vk::CommandBuffer& _buf)
{
    for (auto& cacheRef: descriptorSetCacheReferences)
        cacheRef.keepAlive();


    _buf.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eComputeShader, {}, {}, {}, {
            vk::ImageMemoryBarrier{
                .srcAccessMask = {},
                .dstAccessMask = {},
                .oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
                .newLayout = vk::ImageLayout::eGeneral,
                .image = context.renderer().currentOffscreenData().depthPingImage.image(),
                .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eDepth,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            }
        });

    _buf.bindPipeline(vk::PipelineBindPoint::eCompute, *_depthBlurPipeline);

    auto const& setLayouts = _depthBlurPipeline.descriptor().pipelineLayout.descriptor().setLayouts;
    std::vector<vk::DescriptorSet> descriptorSets;
    for (size_t i = 0; i < 3; ++i)
        descriptorSets.emplace_back(*_descriptorSets.at(3 * context.renderer().currentFrameSync() + i));
    _buf.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        *_depthBlurPipeline.descriptor().pipelineLayout,
        0,
        descriptorSets,
        {}
    );
    _buf.dispatch(1024 / 256,  1024, 1);
}


}
