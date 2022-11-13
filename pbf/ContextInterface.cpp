#include <vulkan/vulkan.hpp>

#include <pbf/ContextInterface.h>
#include <pbf/Cache.h>
#include <pbf/descriptors/DescriptorSet.h>
#include <pbf/descriptors/PipelineLayout.h>

namespace pbf {

void ContextInterface::bindPipeline(
	vk::CommandBuffer buf,
	vk::PipelineBindPoint bindPoint,
	vk::Pipeline pipeline,
	CacheReference<descriptors::PipelineLayout> pipelineLayout,
	std::vector<std::vector<descriptors::DescriptorSetBinding>> const& bindings
)
{
	buf.bindPipeline(bindPoint, pipeline);
	auto const& setLayouts = pipelineLayout.descriptor().setLayouts;
	std::vector<vk::DescriptorSet> descriptorSets;
	size_t numDescriptorSets = std::min(setLayouts.size(), bindings.size());
	for (size_t i = 0; i < numDescriptorSets; ++i)
	{
		descriptorSets.emplace_back(*cache().fetch(
			descriptors::DescriptorSet{
				.setLayout = setLayouts[i],
				.bindings = bindings[i]
			}
		));
	}
	buf.bindDescriptorSets(
		bindPoint,
		*pipelineLayout,
		0,
		descriptorSets,
		{}
	);
}
}