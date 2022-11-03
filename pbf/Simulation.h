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
class ParticleKey;

class Simulation
{
public:
	Simulation(InitContext& context, RingBuffer<ParticleData>& particleData);
	uint32_t getNumParticles() const {
		return _particleData.size();
	}
	void resetKeys() { _resetKeys = true; }
	void run(vk::CommandBuffer buf);

private:
	void initKeys(Context& context, vk::CommandBuffer buf);
	bool _resetKeys = false;
	Context& _context;
	RingBuffer<ParticleData>& _particleData;
	RingBuffer<ParticleKey> _particleKeys;

	struct GridData {
		glm::ivec4 max = glm::ivec4(127, 127, 127, 0);
		glm::ivec4 min = glm::ivec4(-128, -128, -128, 0);
		glm::ivec4 hashweights = glm::ivec4(1, (max.x - min.x + 1), (max.x - min.x + 1) * (max.y - min.y + 1), 0);
		[[nodiscard]] inline size_t numCells() const {
			glm::ivec3 gridExtents = glm::ivec3(max) - glm::ivec3(min);
			return (gridExtents.x + 1) * (gridExtents.y + 1) * (gridExtents.z + 1);
		}
	};
	Buffer<GridData> _gridDataBuffer;

	Buffer<float> _lambdaBuffer;

	RadixSort _radixSort;
	NeighbourCellFinder _neighbourCellFinder;

	std::vector<vk::DescriptorSet> initDescriptorSets;
	std::vector<vk::DescriptorSet> particleDataDescriptorSets;
	std::vector<vk::DescriptorSet> particleKeyDescriptorSets;
	vk::DescriptorSet pingDescriptorSet;
	vk::DescriptorSet pongDescriptorSet;
	std::array<vk::DescriptorSet, 2> neighbourCellFinderInputDescriptorSets;
	vk::DescriptorSet lambdaDescriptorSet;


	RingBuffer<ParticleData> _tempBuffer;
	std::array<vk::DescriptorSet, 2> _tempBufferDescriptorSets;

	CacheReference<descriptors::ComputePipeline> _unconstrainedSystemUpdatePipeline;
	CacheReference<descriptors::ComputePipeline> _particleDataUpdatePipeline;
	CacheReference<descriptors::ComputePipeline> _calcLambdaPipeline;
	CacheReference<descriptors::ComputePipeline> _updatePosPipeline;

	static constexpr std::uint32_t blockSize = 256;
};

}
