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

class Simulation
{
public:
	Simulation(InitContext& context, size_t numParticles, vk::DescriptorBufferInfo input, vk::DescriptorBufferInfo output);
	uint32_t getNumParticles() const {
		return _numParticles;
	}
	void run(vk::CommandBuffer buf);

private:
	Context& _context;
	size_t _numParticles = 0;
	RadixSort _radixSort;

	struct GridData {
		glm::ivec4 max = glm::ivec4(31, 31, 31, 0);
		glm::ivec4 min = glm::ivec4(-32, -32, -32, 0);
		glm::ivec4 hashweights = glm::ivec4(1, (max.x - min.x + 1), (max.x - min.x + 1) * (max.y - min.y + 1), 0);
	};
	Buffer<GridData> gridDataBuffer;

	vk::DescriptorBufferInfo _input;
	vk::DescriptorBufferInfo _output;

	vk::UniqueDescriptorPool _descriptorPool;

	vk::DescriptorSet pingDescriptorSet;
	vk::DescriptorSet pongDescriptorSet;

	static constexpr std::uint32_t blockSize = 256;
};

}


#endif //PBF_SIMULATION_H
