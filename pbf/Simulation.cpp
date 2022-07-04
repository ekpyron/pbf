#include "Simulation.h"
#include "Context.h"
#include "Renderer.h"
#include <pbf/descriptors/ComputePipeline.h>

namespace pbf {

Simulation::Simulation(Context *context):
_context(context)
{
	_particleData = Buffer{
		context,
		sizeof(ParticleData) * _numParticles * context->renderer().framePrerenderCount(),
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
		pbf::MemoryType::STATIC
	};

}

void Simulation::run(vk::CommandBuffer buf)
{
	if (!initialized)
		initialize(buf);

	const auto& computePipeline = _context->cache().fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = _context->cache().fetch(
					pbf::descriptors::ShaderModule{
						.filename = "shaders/simulation.comp.spv",
						PBF_DESC_DEBUG_NAME("shaders/simulation.comp.spv Compute Shader")
					}),
				.entryPoint = "main"
			},
			.pipelineLayout = _context->cache().fetch(
				pbf::descriptors::PipelineLayout{
					.setLayouts = {{
						_context->globalDescriptorSetLayout()
					}},
					PBF_DESC_DEBUG_NAME("Dummy Pipeline Layout")
				}),
			PBF_DESC_DEBUG_NAME("Compute pipeline")
		}
	);
	/*auto buffer = std::move(_context->device().allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
		.commandPool = _context->commandPool(true),
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1U
	}).front());

#ifndef NDEBUG
	{
		static std::size_t __counter{0};
		PBF_DEBUG_SET_OBJECT_NAME(_context, *buffer, fmt::format("Compute Command Buffer #{}", __counter++));
	}
#endif

	buffer->begin(vk::CommandBufferBeginInfo{
		.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
		.pInheritanceInfo = nullptr
	});
*/
	buf.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline);
	buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(computePipeline.descriptor().pipelineLayout), 0, { _context->globalDescriptorSet()}, {});
	buf.dispatch(64*64*64, 1, 1);

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eVertexInput, {}, {}, {vk::BufferMemoryBarrier{
		.srcAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eMemoryWrite,
		.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.buffer = _particleData.buffer(),
		.offset = 0, // sizeof(ParticleData) * _numParticles * (_context->renderer().framePrerenderCount() - 1),
		.size = _particleData.size(), // initializeBuffer.size()
	}}, {});

//	buffer->end();

//	return buffer;
}

void Simulation::initialize(vk::CommandBuffer buf)
{
	Buffer initializeBuffer {_context, sizeof(ParticleData) * _numParticles, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT};

	ParticleData* data = reinterpret_cast<ParticleData*>(initializeBuffer.data());

		for (int32_t x = 0; x < 64; ++x)
			for (int32_t y = 0; y < 64; ++y)
				for (int32_t z = 0; z < 64; ++z)
				{
					auto id = (64 * 64 * x + 64 * y + z);
					data[id].position = glm::vec3(x - 32, y - 32, z - 32);
				}

	initializeBuffer.flush();

	buf.copyBuffer(initializeBuffer.buffer(), _particleData.buffer(), {
		vk::BufferCopy {
			.srcOffset = 0,
			.dstOffset = sizeof(ParticleData) * _numParticles * (_context->renderer().framePrerenderCount() - 1),
			.size = initializeBuffer.size()
		}
	});

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {vk::BufferMemoryBarrier{
		.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask = vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eMemoryRead,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.buffer = _particleData.buffer(),
		.offset = sizeof(ParticleData) * _numParticles * (_context->renderer().framePrerenderCount() - 1),
		.size = initializeBuffer.size(), // initializeBuffer.size()
	}}, {});


	_context->renderer().stage([
									this, initializeBuffer = std::move(initializeBuffer)
								] (vk::CommandBuffer& enqueueBuffer) {
	});

	initialized = true;
}

}