#include "Simulation.h"
#include "Context.h"
#include "Renderer.h"

namespace pbf {

Simulation::Simulation(Context *context):
_context(context)
{
	_particleData = Buffer{
		context,
		sizeof(ParticleData) * _numParticles,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
		pbf::MemoryType::STATIC
	};
}

void Simulation::run()
{
	if (!initialized)
		initialize();
}

void Simulation::initialize()
{
	Buffer initializeBuffer {_context, sizeof(ParticleData) * _numParticles, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT};

	ParticleData* data = reinterpret_cast<ParticleData*>(initializeBuffer.data());

	for (int32_t x = 0; x < 64; ++x)
		for (int32_t y = 0; y < 64; ++y)
			for (int32_t z = 0; z < 64; ++z)
			{
				auto id = 64 * 64 * x + 64 * y + z;
				data[id].position = glm::vec3(x - 32, y - 32, z - 32);
			}

	initializeBuffer.flush();

	_context->renderer().stage([
									this, initializeBuffer = std::move(initializeBuffer)
								] (vk::CommandBuffer& enqueueBuffer) {
		enqueueBuffer.copyBuffer(initializeBuffer.buffer(), _particleData.buffer(), {
			vk::BufferCopy {
				0, 0, _particleData.size()
			}
		});

		enqueueBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexInput, {}, {}, {vk::BufferMemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
			.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.buffer = _particleData.buffer(),
			.offset = 0,
			.size = _particleData.size()
		}}, {});
	});

	initialized = true;
}

}