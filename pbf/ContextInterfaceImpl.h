#pragma once

#include <pbf/Cache.h>
#include <pbf/descriptors/DescriptorSet.h>
#include <pbf/descriptors/PipelineLayout.h>

namespace pbf
{

template<typename PipelineType>
inline void ContextInterface::bindPipeline(
	vk::CommandBuffer buf,
	CacheReference<PipelineType> pipeline,
	std::vector<std::vector<descriptors::DescriptorSetBinding>> const& bindings
)
{
	buf.bindPipeline(PipelineType::bindPoint, *pipeline);
	CacheReference<descriptors::PipelineLayout> pipelineLayout = pipeline.descriptor().pipelineLayout;
	auto const& setLayouts = pipelineLayout.descriptor().setLayouts;
	std::vector<vk::DescriptorSet> descriptorSets;
	for (size_t i = 0; i < setLayouts.size(); ++i)
	{
		descriptorSets.emplace_back(*cache().fetch(
			descriptors::DescriptorSet{
				.setLayout = setLayouts[i],
				.bindings = bindings[i]
			}
		));
	}
	buf.bindDescriptorSets(
		PipelineType::bindPoint,
		*pipelineLayout,
		0,
		descriptorSets,
		{}
	);
}
}