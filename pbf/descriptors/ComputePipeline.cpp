/**
 *
 *
 * @file ComputePipeline.cpp
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#include "ComputePipeline.h"

#include <pbf/VulkanContext.h>

using namespace pbf::descriptors;

pbf::Pipeline ComputePipeline::realize(ContextInterface &context) const {
	Pipeline computePipeline;
	vk::SpecializationInfo specializationInfo;


	computePipeline.pipelineLayout = Pipeline::deducePipelineLayout(context.cache(),
	{ shaderStage }
	PBF_ARG_DEBUG_NAME(debugName + " Pipeline Layout")
	);


	auto [result, pipeline] = context.device().createComputePipelineUnique(nullptr, vk::ComputePipelineCreateInfo{
		.flags = flags,
		.stage = shaderStage.createInfo(specializationInfo),
		.layout = *computePipeline.pipelineLayout,
		.basePipelineHandle = {},
		.basePipelineIndex = {}
	});

	if (result != vk::Result::eSuccess)
		throw std::runtime_error("could not create graphics pipeline.");

	computePipeline.pipeline = std::move(pipeline);
	return computePipeline;
}
