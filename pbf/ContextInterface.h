#pragma once

#include <vulkan/vulkan.hpp>
#include <variant>

namespace pbf {

class MemoryManager;
class Cache;
template<typename>
class CacheReference;
class GUI;

namespace descriptors {
using DescriptorSetBinding = std::variant<vk::DescriptorBufferInfo>;
struct PipelineLayout;
}

class ContextInterface {
public:
	virtual const vk::Device &device() const = 0;
	virtual const vk::PhysicalDevice &physicalDevice() const = 0;
	virtual MemoryManager &memoryManager() = 0;
	virtual Cache &cache() = 0;
	virtual vk::DescriptorPool descriptorPool() const = 0;


	template<typename PipelineType>
	void bindPipeline(
		vk::CommandBuffer buf,
		CacheReference<PipelineType> pipeline,
		std::vector<std::vector<descriptors::DescriptorSetBinding>> const& bindings
	) {
		bindPipeline(buf, PipelineType::bindPoint, *pipeline, pipeline.descriptor().pipelineLayout, bindings);
	}

	void bindPipeline(
		vk::CommandBuffer buf,
		vk::PipelineBindPoint bindPoint,
		vk::Pipeline pipeline,
		CacheReference<descriptors::PipelineLayout> pipelineLayout,
		std::vector<std::vector<descriptors::DescriptorSetBinding>> const& bindings
	);


#ifndef NDEBUG
	virtual void setGenericObjectName(vk::ObjectType type, uint64_t obj, const std::string &name) const = 0;
#endif

};

}