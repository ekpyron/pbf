#pragma once

#include <vector>

#include <pbf/common.h>
#include <pbf/Cache.h>
#include <pbf/descriptors/Order.h>
#include <pbf/descriptors/DescriptorSetLayout.h>

namespace pbf::descriptors {

using DescriptorSetBinding = std::variant<vk::DescriptorBufferInfo>;

// TODO: support arrays like this:
/*struct DescriptorSetArrayBinding {
	std::vector<DescriptorSetBinding> arrayElements;
};*/

struct DescriptorSet {
	vk::UniqueDescriptorSet realize(ContextInterface& context) const;

	CacheReference<DescriptorSetLayout> setLayout;

	std::vector<DescriptorSetBinding> bindings;

#ifndef NDEBUG
	std::string debugName;
#endif
private:
	using T = DescriptorSet;
public:
	using Compare = PBFMemberComparator<&T::setLayout, &T::bindings>;

};
}
