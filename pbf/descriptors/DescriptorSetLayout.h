/**
 *
 *
 * @file DescriptorSetLayout.h
 * @brief 
 * @author clonker
 * @date 2/8/19
 */
#pragma once

#include <vector>

#include <pbf/common.h>
#include <pbf/descriptors/Order.h>


namespace pbf::descriptors {


struct DescriptorSetLayout {

    // todo: at some point >=< push constants
    vk::UniqueDescriptorSetLayout realize(ContextInterface *context) const;

    struct Binding {
        std::uint32_t binding;
        vk::DescriptorType descriptorType;
        std::uint32_t descriptorCount;
        vk::ShaderStageFlags stageFlags;
        //mutable const std::vector<CacheReference<Sampler>> immutableSamplers; // todo immutable samplers

        template<typename T = Binding>
        using Compare = PBFMemberComparator<&T::binding, &T::descriptorType, &T::descriptorCount, &T::stageFlags>;
    };

    template<typename T = DescriptorSetLayout>
    using Compare = PBFMemberComparator<&T::createFlags, &T::bindings>;

    vk::DescriptorSetLayoutCreateFlags createFlags;
    vector32<Binding> bindings;
#ifndef NDEBUG
    std::string debugName;
#endif

};
}
