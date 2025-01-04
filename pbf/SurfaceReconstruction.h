//
// Created by daniel on 12/29/24.
//

#pragma once

#include <pbf/common.h>
#include <pbf/Cache.h>
#include <pbf/descriptors/ComputePipeline.h>

#include "Buffer.h"
#include "Image.h"
#include "FrameSyncData.h"

namespace pbf
{
struct GlobalAppData;

class SurfaceReconstruction
{
public:
    SurfaceReconstruction(InitContext& _context, Renderer& renderer, GlobalAppData& globalAppData);
    ~SurfaceReconstruction() = default;

    void run(vk::CommandBuffer& _cmdBuffer);

    struct FrameData
    {
        FrameData(InitContext& context, Renderer& renderer, GlobalAppData& globalAppData, SurfaceReconstruction& _parent);
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

        struct DepthBlurDescriptorSets
        {
            vk::UniqueDescriptorSet inputSampler;
            vk::UniqueDescriptorSet outputStorageImage;
            vk::UniqueDescriptorSet blurDirUniformBuffer;
            std::vector<vk::DescriptorSet> all() const
            {
                return {*inputSampler, *outputStorageImage, *blurDirUniformBuffer};
            }
        };
        DepthBlurDescriptorSets depthBlurDescriptorSets;
        DepthBlurDescriptorSets depthBlurPongDescriptorSets;
        struct ReconstructNormalDescriptorSets
        {
            vk::UniqueDescriptorSet inputSampler;
            vk::UniqueDescriptorSet outputStorageImage;
            vk::DescriptorSet uniformBuffer;
            std::vector<vk::DescriptorSet> all() const
            {
                return {*inputSampler, *outputStorageImage, uniformBuffer};
            }
        };
        ReconstructNormalDescriptorSets reconstructNormalDescriptorSets;
    };

    FrameData& frameData() { return frameSyncData.getCurrent(); }

private:

    struct BlurDir
    {
        glm::vec2 direction{};
    };

    VulkanContext& context;

    FrameSyncData<FrameData> frameSyncData;

    vk::UniqueSampler depthSampler;
    Buffer<BlurDir> blurDirBuffer;
    CacheReference<descriptors::ComputePipeline> _depthBlurPipeline;
    CacheReference<descriptors::ComputePipeline> _reconstructNormalsPipeline;
    std::vector<CacheReference<descriptors::DescriptorSetLayout>> descriptorSetCacheReferences;
};

}
