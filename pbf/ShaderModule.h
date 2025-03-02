#pragma once
#include <pbf/common.h>

#include "descriptors/DescriptorSet.h"

namespace pbf
{

struct ShaderModule {
    vk::UniqueShaderModule shaderModule;

    struct DescriptorSetInfo
    {
        uint32_t set;
        descriptors::DescriptorSetLayout descriptorSetLayout;
    };
    struct PushConstantInfo
    {
        uint32_t offset{};
        uint32_t size{};
    };
    vk::ShaderStageFlagBits stageFlags;
    std::vector<DescriptorSetInfo> descriptorSetInfos;
    std::vector<PushConstantInfo> pushConstantInfos;
    std::string entryPoint;

    bool operator!() const { return !shaderModule; }
};
using ShaderModulePtr = std::unique_ptr<ShaderModule>;

}
