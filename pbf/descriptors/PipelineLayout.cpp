/**
 *
 *
 * @file PipelineLayout.cpp
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#include "PipelineLayout.h"
#include <pbf/Context.h>

using namespace pbf::descriptors;

vk::UniquePipelineLayout PipelineLayout::realize(Context *context) const {
    const auto &device = context->device();
    std::vector<vk::DescriptorSetLayout> vkSetLayouts;
    vkSetLayouts.reserve(setLayouts.size());
    std::transform(setLayouts.begin(), setLayouts.end(), std::back_inserter(vkSetLayouts), [](const auto& _layout) {
        return *_layout;
    });
    return device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{
		.setLayoutCount = static_cast<uint32_t>(vkSetLayouts.size()),
		.pSetLayouts = vkSetLayouts.data(),
		.pushConstantRangeCount = pushConstants.size(),
		.pPushConstantRanges = pushConstants.data()
    });
}
