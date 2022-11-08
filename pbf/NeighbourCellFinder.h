#pragma once

#include <pbf/common.h>
#include <pbf/descriptors/ComputePipeline.h>
#include <pbf/Buffer.h>

namespace pbf {

class NeighbourCellFinder
{
public:
	NeighbourCellFinder(ContextInterface& context, size_t numGridCells, size_t maxID);


	static constexpr auto inputDescriptorSetLayout() {
		return descriptors::DescriptorSetLayout{
			.createFlags = {},
			.bindings = {
				{
					.binding = 0,
					.descriptorType = vk::DescriptorType::eStorageBuffer,
					.descriptorCount = 1,
					.stageFlags = vk::ShaderStageFlagBits::eCompute
				},
				{
					.binding = 1,
					.descriptorType = vk::DescriptorType::eUniformBuffer,
					.descriptorCount = 1,
					.stageFlags = vk::ShaderStageFlagBits::eCompute
				}
			},
			PBF_DESC_DEBUG_NAME("Neighbour Cell Finder Input Descriptor Set Layout")
		};
	}

	void operator()(vk::CommandBuffer buf, uint32_t numParticles, vk::DescriptorSet input) const;

	struct alignas(glm::uvec4) GridBoundaries {
		uint startIndex = 0;
		uint endIndex = 0;
	};

	vk::DescriptorSet gridData() const {
		return _gridData;
	}

	const Buffer<GridBoundaries>& gridBoundaryBuffer() const {
		return _gridBoundaryBuffer;
	}

private:
	static constexpr uint32_t blockSize = 256;

	Buffer<GridBoundaries> _gridBoundaryBuffer;

	CacheReference<descriptors::ComputePipeline> _findCellsPipeline;
	CacheReference<descriptors::ComputePipeline> _testPipeline;
	vk::DescriptorSet _gridData;


};

}
