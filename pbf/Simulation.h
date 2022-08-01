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
	Buffer<uint32_t> _particleSortKeys;

	vk::DescriptorBufferInfo _input;
	vk::DescriptorBufferInfo _output;

	vk::UniqueDescriptorPool _descriptorPool;

	CacheReference<descriptors::ComputePipeline> _prescanPipeline;
	CacheReference<descriptors::DescriptorSetLayout> _prescanParamsLayout;
	vk::DescriptorSet _prescanParams;


	struct ScanStage {
		Buffer<std::uint32_t>buffer;
		vk::DescriptorSet params; // TODO rename
		vk::DescriptorSet addBlockSumsParams;
	};

	std::vector<ScanStage> _scanStages;

	CacheReference<descriptors::ComputePipeline> _scanPipeline;
	CacheReference<descriptors::DescriptorSetLayout> _scanParamsLayout;

	CacheReference<descriptors::ComputePipeline> _assignPipeline;
	vk::DescriptorSet _assignParams;
	CacheReference<descriptors::DescriptorSetLayout> _assignParamsLayout;

	CacheReference<descriptors::ComputePipeline> _addBlockSumsPipeline;
	CacheReference<descriptors::DescriptorSetLayout> _addBlockSumsParamsLayout;

	static constexpr std::uint32_t blockSize = 64;

	bool initialized = false;
	void initialize(vk::CommandBuffer buf);
};

}


#endif //PBF_SIMULATION_H
