/**
 *
 *
 * @file PipelineLayout.h
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#pragma once

#include <vector>

#include <pbf/common.h>
#include <pbf/Cache.h>

#include "Order.h"
#include "DescriptorSetLayout.h"

namespace pbf::descriptors {

struct PipelineLayout {

    vk::UniquePipelineLayout realize(ContextInterface *context) const;

    std::vector<CacheReference<DescriptorSetLayout>> setLayouts;
    std::vector<vk::PushConstantRange> pushConstants;
#ifndef NDEBUG
    std::string debugName;
#endif

private:
	using T = PipelineLayout;
public:
	using Compare = PBFMemberComparator<&T::setLayouts, &T::pushConstants>;
};

}
