#pragma once

#include "common.h"
#include "UIControlled.h"
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

class Simulation: public UIControlled
{
public:
	Simulation(InitContext& context, RingBuffer<ParticleData>& particleData);
	uint32_t getNumParticles() const {
		return _particleData.size();
	}
	void resetKeys() { _resetKeys = true; }
	void run(vk::CommandBuffer buf, float timestep);
protected:
	void ui() override;
private:

	void buildPipelines();

	float h = glm::length(glm::vec3(1.0f, 1.0f, 1.0f));
	float rho_0 = 1.0f;
	float epsilon = 5.0f;
	float xsph_viscosity_c = 0.01f;
	float tensile_instability_k = 0.1f;
	float vorticity_epsilon = 5.0f;

	float _lastTimestep = 1.0 / 60.0;
	void initKeys(Context& context, vk::CommandBuffer buf);
	bool _resetKeys = false;
	Context& _context;
	RingBuffer<ParticleData>& _particleData;
	RingBuffer<ParticleKey> _particleKeys;

	struct UnconstrainedPositionUpdatePushConstants {
		glm::vec3 externalForces;
		float lastTimestep = 1.0f/60.0f;
		float timestep = 1.0f/60.0f;
	};

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
	Buffer<glm::vec4> _vorticityBuffer;

	RadixSort _radixSort;
	NeighbourCellFinder _neighbourCellFinder;

	RingBuffer<ParticleKey> _tempBuffer;

	CacheReference<descriptors::ComputePipeline> _unconstrainedSystemUpdatePipeline;
	CacheReference<descriptors::ComputePipeline> _particleDataUpdatePipeline;
	CacheReference<descriptors::ComputePipeline> _calcLambdaPipeline;
	CacheReference<descriptors::ComputePipeline> _updatePosPipeline;
	CacheReference<descriptors::ComputePipeline> _calcVorticityPipeline;
	CacheReference<descriptors::ComputePipeline> _updateVelPipeline;

	descriptors::ShaderStage::SpecializationInfo makeSpecializationInfo() const;

	static constexpr std::uint32_t blockSize = 256;
};

}
