//
// Created by daniel on 04.07.22.
//

#ifndef PBF_SIMULATION_H
#define PBF_SIMULATION_H

#include "common.h"
#include "Buffer.h"
#include "Cache.h"

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

	struct ScanStage {
		Buffer buffer;
		CacheReference<descriptors::DescriptorSetLayout> paramsLayout;
		vk::DescriptorSet params;
	};

	std::vector<ScanStage> _scanStages;

	static constexpr std::uint32_t blockSize = 256;

	struct PushConstantData {
		uint sourceIndex;
		uint destIndex;
		uint numParticles;
	};

	vk::UniqueDescriptorPool _descriptorPool;
	CacheReference<descriptors::DescriptorSetLayout> _prescanParamsLayout;
	vk::DescriptorSet _prescanParams;

	bool initialized = false;
	void initialize(vk::CommandBuffer buf);
};

}


#endif //PBF_SIMULATION_H
