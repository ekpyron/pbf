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
		if (setLayouts[i].valid())
		{
			assert(setLayouts[i].descriptor().bindings.size() <= bindings[i].size());
			descriptors::DescriptorSet descriptorSetDescriptor{
				.setLayout = setLayouts[i],
				.bindings = bindings[i]
			};
			descriptorSetDescriptor.bindings.resize(setLayouts[i].descriptor().bindings.size());
			descriptorSets.emplace_back(*cache().fetch(descriptorSetDescriptor));
		}
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