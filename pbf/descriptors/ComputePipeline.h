#pragma once

#include <pbf/common.h>
#include <pbf/Cache.h>
#include <pbf/descriptors/ShaderStage.h>
#include <pbf/descriptors/ShaderModule.h>
#include <pbf/descriptors/PipelineLayout.h>
#include <pbf/descriptors/RenderPass.h>

namespace pbf::descriptors {

struct ComputePipeline {
    vk::UniquePipeline realize(ContextInterface* context) const;

	vk::PipelineCreateFlags flags;
	ShaderStage shaderStage;

	CacheReference<PipelineLayout> pipelineLayout;


private:
	using T = ComputePipeline;
public:
    using Compare = PBFMemberComparator<&T::flags, &T::shaderStage, &T::pipelineLayout>;

#ifndef NDEBUG
    std::string debugName;
#endif

    template<typename T = GraphicsPipeline>
    using Depends = crampl::NonTypeList<&T::renderPass>;
};

}
