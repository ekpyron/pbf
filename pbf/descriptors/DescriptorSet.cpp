#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "../Context.h"
namespace pbf::descriptors {

// TODO: deduplicate
template<typename... Args> struct LambdaVisitor : Args... { using Args::operator()...; };
template<typename... Args> LambdaVisitor(Args...) -> LambdaVisitor<Args...>;

vk::UniqueDescriptorSet DescriptorSet::realize(ContextInterface &context) const {
	vk::DescriptorSetLayout layout = *setLayout;
	auto result = std::move(context.device().allocateDescriptorSetsUnique(
		vk::DescriptorSetAllocateInfo{
			.descriptorPool = context.descriptorPool(),
			.descriptorSetCount = 1,
			.pSetLayouts = &layout
		}
	).front());

	std::vector<vk::WriteDescriptorSet> writes;

	for(uint32_t i = 0; i < size32(bindings); ++i) {
		std::visit(
			LambdaVisitor{
				[&](vk::DescriptorBufferInfo const& bufferInfo) {
					writes.emplace_back(vk::WriteDescriptorSet{
						.dstSet = *result,
						.dstBinding = i,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = setLayout.descriptor().bindings[i].descriptorType,
						.pImageInfo = nullptr,
						.pBufferInfo = &bufferInfo,
						.pTexelBufferView = nullptr
					});
				}
				// TODO: other cases.
			},
			bindings[i]
		);
	}
	context.device().updateDescriptorSets(writes, {});
	return result;
}

}
