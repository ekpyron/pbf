#pragma once

#include "common.h"
#include "Buffer.h"
#include "Cache.h"
#include "RadixSort.h"
#include "NeighbourCellFinder.h"
#include <pbf/descriptors/ComputePipeline.h>

namespace pbf {

class Context;
class InitContext;
class ParticleData;

class Simulation
{
public:
	Simulation(InitContext& context, RingBuffer<ParticleData>& particleData);
	uint32_t getNumParticles() const {
		return _particleData.size();
	}
	void run(vk::CommandBuffer buf);

private:
	Context& _context;
	RingBuffer<ParticleData>& _particleData;

	struct GridData {
		glm::ivec4 max = glm::ivec4(31, 31, 31, 0);
		glm::ivec4 min = glm::ivec4(-32, -32, -32, 0);
		glm::ivec4 hashweights = glm::ivec4(1, (max.x - min.x + 1), (max.x - min.x + 1) * (max.y - min.y + 1), 0);
		[[nodiscard]] inline size_t numCells() const {
			glm::ivec3 gridExtents = glm::ivec3(max) - glm::ivec3(min);
			return (gridExtents.x + 1) * (gridExtents.y + 1) * (gridExtents.z + 1);
		}
	};
	Buffer<GridData> _gridDataBuffer;


	RadixSort _radixSort;
	NeighbourCellFinder _neighbourCellFinder;

	std::vector<vk::DescriptorSet> initDescriptorSets;
	vk::DescriptorSet pingDescriptorSet;
	vk::DescriptorSet pongDescriptorSet;
	vk::DescriptorSet neighbourCellFinderInputDescriptorSet;

	RingBuffer<ParticleData> _tempBuffer;

	CacheReference<descriptors::ComputePipeline> _unconstrainedSystemUpdatePipeline;

	static constexpr std::uint32_t blockSize = 256;
};

}
