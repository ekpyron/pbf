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
    vk::UniqueDescriptorSetLayout realize(ContextInterface &context) const;

    struct Binding {
        std::uint32_t binding = 0;
        vk::DescriptorType descriptorType;
        std::uint32_t descriptorCount;
        vk::ShaderStageFlags stageFlags;
        //mutable const std::vector<CacheReference<Sampler>> immutableSamplers; // todo immutable samplers

	private:
		using T = Binding;
	public:
        using Compare = PBFMemberComparator<&T::binding, &T::descriptorType, &T::descriptorCount, &T::stageFlags>;
    };

    DescriptorSetLayout operator+(const DescriptorSetLayout &_other) const
    {
        DescriptorSetLayout result;
        result.createFlags = createFlags;
        assert(createFlags == _other.createFlags);
        result.bindings.resize(std::max(bindings.size(), _other.bindings.size()));
        for (size_t i = 0; i < result.bindings.size(); ++i)
        {
            result.bindings[i] = (i < bindings.size()) ? bindings[i] : _other.bindings[i];
            if (i < _other.bindings.size() && i < bindings.size())
            {
                auto& b1 = result.bindings[i];
                auto& b2 = _other.bindings[i];
                assert(b1.binding == b2.binding);
                assert(b1.descriptorType == b2.descriptorType);
                assert(b1.descriptorCount == b2.descriptorCount);
                b1.stageFlags = vk::ShaderStageFlagBits::eAll;  // b1.stageFlags|b2.stageFlags;
            }
            result.bindings[i].stageFlags = vk::ShaderStageFlagBits::eAll;
        }
        return result;
    }

    void log() const
    {
        spdlog::get("console")->debug("Create Flags: {}", static_cast<uint32_t>(createFlags));
#ifndef NDEBUG
        spdlog::get("console")->debug("Debug Name: {}", debugName);
#endif
        for (auto& binding : bindings)
        {
        spdlog::get("console")->debug("Binding: {} {} {} {}", binding.binding, static_cast<uint32_t>(binding.descriptorType), binding.descriptorCount, static_cast<uint32_t>(binding.stageFlags));
        }
    }

    vk::DescriptorSetLayoutCreateFlags createFlags;
    std::vector<Binding> bindings;
#ifndef NDEBUG
    std::string debugName;
#endif

private:
	using T = DescriptorSetLayout;
public:
	using Compare = PBFMemberComparator<&T::createFlags, &T::bindings>;

};
}
