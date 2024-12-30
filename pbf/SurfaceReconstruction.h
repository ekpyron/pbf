//
// Created by daniel on 12/29/24.
//

#pragma once

#include <pbf/common.h>
#include <pbf/Cache.h>
#include <pbf/descriptors/ComputePipeline.h>

#include "Buffer.h"

namespace pbf
{

class SurfaceReconstruction
{
public:
    SurfaceReconstruction(Context& _context);
    ~SurfaceReconstruction() = default;

    void run(vk::CommandBuffer& _cmdBuffer);

    void initDescriptorSets();

private:
    struct BlurDir
    {
        glm::vec2 direction{};
    };

    Context& context;
    vk::UniqueSampler depthSampler;
    Buffer<BlurDir> blurDirBuffer;
    CacheReference<descriptors::ComputePipeline> _depthBlurPipeline;
    std::vector<CacheReference<descriptors::DescriptorSetLayout>> descriptorSetCacheReferences;
    std::vector<vk::UniqueDescriptorSet> _descriptorSets;

};

}
