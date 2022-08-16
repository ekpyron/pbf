#pragma once

#include "Buffer.h"

#include <cstdint>
#include <vector>

namespace pbf {

namespace descriptors {
class DescriptorSetLayout;
class ShaderModule;
}

class ContextInterface;
class Cache;

class RadixSort
{
public:
	RadixSort(
		ContextInterface& _context,
		Cache& _cache,
		uint32_t _blockSize,
		uint32_t _numBlocks,
		descriptors::DescriptorSetLayout const& _keyAndGlobalSortShaderDescriptors,
		descriptors::ShaderModule const& _keyShaderModule,
		descriptors::ShaderModule const& _globalSortShaderModule
	);
	RadixSort(const RadixSort&) = delete;
	~RadixSort() = default;
	RadixSort& operator=(const RadixSort&) = delete;
private:
	ContextInterface& context;
	Cache& cache;

	const uint32_t blockSize = 0;
	const uint32_t numBlocks = 0;
	const uint32_t numKeys = blockSize * numBlocks;

	Buffer<uint32_t> prefixSums;
	std::vector<Buffer<uint32_t>> blockSums;

	struct PushConstants {
		glm::u32vec4 blockSumOffset;
		uint32_t bit = 0;
	};
};

}

