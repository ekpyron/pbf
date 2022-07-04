//
// Created by daniel on 04.07.22.
//

#ifndef PBF_SIMULATION_H
#define PBF_SIMULATION_H

#include "common.h"
#include "Buffer.h"

namespace pbf {

class Context;

class Simulation
{
public:
	Simulation(Context* context);
	uint32_t getNumParticles() const {
		return _numParticles;
	}
	void run(vk::CommandBuffer buf);
	Buffer& particleData() { return _particleData; }
	const Buffer& particleData() const { return _particleData; }

	struct alignas(sizeof(glm::vec4)) ParticleData {
		glm::vec3 position;
	};
private:
	Context* _context;
	Buffer _particleData;
	uint32_t _numParticles = 64 * 64 * 64;

	bool initialized = false;
	void initialize(vk::CommandBuffer buf);
};

}


#endif //PBF_SIMULATION_H
