//
// Created by daniel on 04.07.22.
//

#ifndef PBF_SIMULATION_H
#define PBF_SIMULATION_H

#include "common.h"
#include "Buffer.h"
#include "Cache.h"
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
	Buffer _particleSortKeys;

	vk::DescriptorBufferInfo _input;
	vk::DescriptorBufferInfo _output;

	CacheReference<descriptors::ComputePipeline> _prescanPipeline;
	CacheReference<descriptors::ComputePipeline> _scanPipeline;
	CacheReference<descriptors::DescriptorSetLayout> _scanParamsLayout;

	Buffer _prescanBlocksums;
	struct ScanStage {
		Buffer buffer;
		vk::DescriptorSet params;
	};

	std::vector<ScanStage> _scanStages;

	static constexpr std::uint32_t blockSize = 256;

	vk::UniqueDescriptorPool _descriptorPool;
	CacheReference<descriptors::DescriptorSetLayout> _prescanParamsLayout;
	vk::DescriptorSet _prescanParams;

	bool initialized = false;
	void initialize(vk::CommandBuffer buf);
};

}


#endif //PBF_SIMULATION_H
