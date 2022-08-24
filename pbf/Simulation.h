//
// Created by daniel on 04.07.22.
//

#ifndef PBF_SIMULATION_H
#define PBF_SIMULATION_H

#include "common.h"
#include "Buffer.h"
#include "Cache.h"
#include "RadixSort.h"
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
	RadixSort _radixSort;

	struct GridData {
		glm::ivec4 max = glm::ivec4(31, 31, 31, 0);
		glm::ivec4 min = glm::ivec4(-32, -32, -32, 0);
		glm::ivec4 hashweights = glm::ivec4(1, (max.x - min.x + 1), (max.x - min.x + 1) * (max.y - min.y + 1), 0);
	};
	Buffer<GridData> _gridDataBuffer;

	vk::UniqueDescriptorPool _descriptorPool;

	std::vector<vk::DescriptorSet> initDescriptorSets;
	vk::DescriptorSet pingDescriptorSet;
	vk::DescriptorSet pongDescriptorSet;

	RingBuffer<ParticleData> _tempBuffer;


	static constexpr std::uint32_t blockSize = 256;
};

}


#endif //PBF_SIMULATION_H
