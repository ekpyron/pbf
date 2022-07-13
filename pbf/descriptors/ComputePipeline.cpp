/**
 *
 *
 * @file ComputePipeline.cpp
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#include "ComputePipeline.h"

#include <pbf/Context.h>

using namespace pbf::descriptors;

vk::UniquePipeline ComputePipeline::realize(ContextInterface* context) const {
	vk::SpecializationInfo specializationInfo;
	auto [result, pipeline] = context->device().createComputePipelineUnique(nullptr, vk::ComputePipelineCreateInfo{
		.flags = flags,
		.stage = shaderStage.createInfo(specializationInfo),
		.layout = *pipelineLayout,
		.basePipelineHandle = {},
		.basePipelineIndex = {}
	});

	if (result != vk::Result::eSuccess)
		throw std::runtime_error("could not create graphics pipeline.");
	return std::move(pipeline);
}
