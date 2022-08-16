#include "Simulation.h"
#include "Context.h"
#include "Renderer.h"

namespace pbf {

Simulation::Simulation(Context *context, size_t numParticles, vk::DescriptorBufferInfo input, vk::DescriptorBufferInfo output):
_context(context), _numParticles(numParticles),
_input(input), _output(output)
{
	if (getNumParticles() % blockSize)
		throw std::runtime_error("Particle count must be a multiple of blockSize.");
	if (!getNumParticles())
		throw std::runtime_error("Need at least block size particles.");

	{
		std::uint32_t numDescriptorSets = 512;
		static std::array<vk::DescriptorPoolSize, 1> sizes {{
			{ vk::DescriptorType::eStorageBuffer, 1024 }
		}};
		_descriptorPool = context->device().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
			.maxSets = numDescriptorSets,
			.poolSizeCount = static_cast<std::uint32_t>(sizes.size()),
			.pPoolSizes = sizes.data()
		});
	}

}

void Simulation::run(vk::CommandBuffer buf)
{
	if (!_initialized)
		initialize(buf);

}

void Simulation::initialize(vk::CommandBuffer buf)
{

	_initialized = true;
}

}