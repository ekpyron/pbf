#pragma once

#include "Buffer.h"
#include "Cache.h"

#include <cstdint>
#include <vector>

namespace pbf {

namespace descriptors {
class DescriptorSetLayout;
class ShaderModule;
class ComputePipeline;
using DescriptorSetBinding = std::variant<vk::DescriptorBufferInfo>;
}

class ContextInterface;

class RadixSort
{
public:
	enum class Result {
		InPingBuffer,
		InPongBuffer
	};
	RadixSort(
		ContextInterface& _context,
		uint32_t _blockSize,
		uint32_t _numBlocks,
		std::vector<descriptors::DescriptorSetLayout> const& _keyAndGlobalSortShaderDescriptorLayouts,
		std::string_view _shaderPrefix
	);
	RadixSort(const RadixSort&) = delete;
	RadixSort(RadixSort&&) = default;
	~RadixSort() = default;
	RadixSort& operator=(const RadixSort&) = delete;

	Result stage(
		vk::CommandBuffer buf,
		uint32_t _sortBits,
		std::vector<descriptors::DescriptorSetBinding> initInfos,
		std::vector<descriptors::DescriptorSetBinding> pingInfos,
		std::vector<descriptors::DescriptorSetBinding> pongInfos
	) const;
private:
	ContextInterface& context;

	const uint32_t blockSize = 0;
	const uint32_t numBlocks = 0;
	const uint32_t numKeys = blockSize * numBlocks;

	Buffer<uint32_t> prefixSums;
	std::vector<Buffer<uint32_t>> blockSums;

	CacheReference<descriptors::ComputePipeline> prescanPipeline;
	CacheReference<descriptors::ComputePipeline> scanPipeline;
	CacheReference<descriptors::ComputePipeline> addBlockSumPipeline;
	CacheReference<descriptors::ComputePipeline> globalSortPipeline;

	vk::DescriptorSet prescanParams;
	std::vector<vk::DescriptorSet> scanParams;
	vk::DescriptorSet globalSortParams;

	struct PushConstants {
		glm::u32vec4 blockSumOffset;
		uint32_t bit = 0;
	};
};

}

