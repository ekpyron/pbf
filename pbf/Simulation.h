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
	Simulation(InitContext& context, size_t numParticles);
	uint32_t getNumParticles() const {
		return _particleData.size();
	}
    void reset(vk::CommandBuffer& buf);
	void resetKeys() { _resetKeys = true; }
	void run(vk::CommandBuffer buf, float timestep);
    void copy(vk::CommandBuffer buf, vk::Buffer dst, size_t dstOffset);
protected:
	void ui() override;
	std::string uiCategory() const override;
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
    size_t ringBufferIndex = 0;
    size_t nextRingBufferIndex() const {
        return (ringBufferIndex + 1) % _particleData.segments();
    }
    size_t previousRingBufferIndex() const {
        return (ringBufferIndex + (_particleData.segments() - 1)) % _particleData.segments();
    }
	RingBuffer<ParticleData> _particleData;
	RingBuffer<ParticleKey> _particleKeys;

	struct UnconstrainedPositionUpdatePushConstants {
		glm::vec3 externalAccell;
		float lastTimestep = 1.0f/60.0f;
		float timestep = 1.0f/60.0f;
	};

    using GridData = NeighbourCellFinder::GridData;
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
