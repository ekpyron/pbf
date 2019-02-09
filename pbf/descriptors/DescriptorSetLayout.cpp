/**
 *
 *
 * @file DescriptorSetLayout.cpp
 * @brief 
 * @author clonker
 * @date 2/8/19
 */
#include "DescriptorSetLayout.h"
#include "../Context.h"
namespace pbf::descriptors {

vk::UniqueDescriptorSetLayout DescriptorSetLayout::realize(Context *context) const {
    std::vector<vk::DescriptorSetLayoutBinding> vkBindings;
    vkBindings.reserve(bindings.size());
    std::transform(bindings.begin(), bindings.end(), std::back_inserter(vkBindings), [](const Binding &binding) {
        return vk::DescriptorSetLayoutBinding {
            binding.binding,
            binding.descriptorType,
            binding.descriptorCount,
            binding.stageFlags,
            nullptr // todo: immutable samplers
        };
    });
    return context->device().createDescriptorSetLayoutUnique({
        createFlags, bindings.size(), vkBindings.data()
    });
}
}
