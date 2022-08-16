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

class Simulation
{
public:
	Simulation(Context* context, size_t numParticles, vk::DescriptorBufferInfo input, vk::DescriptorBufferInfo output);
	uint32_t getNumParticles() const {
		return _numParticles;
	}
	void run(vk::CommandBuffer buf);

private:
	Context* _context;
	size_t _numParticles = 0;
//	RadixSort _radixSort;

	vk::DescriptorBufferInfo _input;
	vk::DescriptorBufferInfo _output;

	vk::UniqueDescriptorPool _descriptorPool;

	static constexpr std::uint32_t blockSize = 256;

	bool _initialized = false;
	void initialize(vk::CommandBuffer buf);
};

}


#endif //PBF_SIMULATION_H
