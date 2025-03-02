#pragma once

#include <pbf/common.h>
#include <pbf/Cache.h>
#include <pbf/descriptors/ShaderStage.h>
#include <pbf/descriptors/ShaderModule.h>
#include <pbf/descriptors/PipelineLayout.h>
#include <pbf/descriptors/RenderPass.h>
#include "../Pipeline.h"

namespace pbf::descriptors {

struct ComputePipeline {
    Pipeline realize(ContextInterface &context) const;

	vk::PipelineCreateFlags flags;
	ShaderStage shaderStage;

	static constexpr vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eCompute;
private:
	using T = ComputePipeline;
public:
    using Compare = PBFMemberComparator<&T::flags, &T::shaderStage>;

#ifndef NDEBUG
    std::string debugName;
#endif

    template<typename T = GraphicsPipeline>
    using Depends = crampl::NonTypeList<&T::renderPass>;
};

}
