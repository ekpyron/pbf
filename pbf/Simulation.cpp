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
	_particleSortKeys = Buffer{
		context,
		32 * _numParticles * context->renderer().framePrerenderCount(),
		vk::BufferUsageFlagBits::eStorageBuffer,
		pbf::MemoryType::STATIC
	};
	{
		static std::array<vk::DescriptorPoolSize, 1> sizes {{
			{ vk::DescriptorType::eStorageBuffer, 2 }
		}};
		_descriptorPool = context->device().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
			.maxSets = numDescriptorSets,
			.poolSizeCount = static_cast<std::uint32_t>(sizes.size()),
			.pPoolSizes = sizes.data()
		});
	}

	_paramsLayout = context->cache().fetch(descriptors::DescriptorSetLayout{
		.createFlags = {},
		.bindings = {{
						 .binding = 0,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1,
						 .stageFlags = vk::ShaderStageFlagBits::eCompute
					 },
					 {
						 .binding = 1,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1,
						 .stageFlags = vk::ShaderStageFlagBits::eCompute
					 }}
#ifndef NDEBUG
		,.debugName = "Simulation Descriptor Set Layout"
#endif
	});
	_params = context->device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = *_descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &*_paramsLayout
	}).front();
	vk::DescriptorBufferInfo particleDataBufferDescriptorInfo {
		particleData().buffer(), 0, particleData().size()
	};
	vk::DescriptorBufferInfo particleSortKeysBufferDescriptorInfo {
		_particleSortKeys.buffer(), 0, _particleSortKeys.size()
	};

	context->device().updateDescriptorSets({vk::WriteDescriptorSet{
		.dstSet = _params,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.pImageInfo = nullptr,
		.pBufferInfo = &particleDataBufferDescriptorInfo,
		.pTexelBufferView = nullptr
	}, vk::WriteDescriptorSet{
		.dstSet = _params,
		.dstBinding = 1,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.pImageInfo = nullptr,
		.pBufferInfo = &particleSortKeysBufferDescriptorInfo,
		.pTexelBufferView = nullptr
	}}, {});
}

void Simulation::run(vk::CommandBuffer buf)
{
	if (!initialized)
		initialize(buf);

	_paramsLayout.keepAlive();
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
						_paramsLayout
					}},
					.pushConstants = {vk::PushConstantRange{
						.stageFlags = vk::ShaderStageFlagBits::eCompute,
						.offset = 0,
						.size = sizeof(PushConstantData)
					}},
					PBF_DESC_DEBUG_NAME("Simulation Pipeline Layout")
				}),
			PBF_DESC_DEBUG_NAME("Compute pipeline layout")
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

	PushConstantData pushConstantData{
		.sourceIndex = _context->renderer().previousFrameSync(),
		.destIndex = _context->renderer().currentFrameSync(),
		.numParticles = getNumParticles()
	};

	buf.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline);
	buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(computePipeline.descriptor().pipelineLayout), 0, { _params}, {});
	buf.pushConstants(
		*computePipeline.descriptor().pipelineLayout,
		vk::ShaderStageFlagBits::eCompute,
		0,
		sizeof(PushConstantData),
		&pushConstantData
	);
	buf.dispatch((getNumParticles() + 255)/256, 1, 1);

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eVertexInput, {}, {}, {vk::BufferMemoryBarrier{
		.srcAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eMemoryWrite,
		.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.buffer = _particleData.buffer(),
		.offset = sizeof(ParticleData) * _numParticles * (_context->renderer().framePrerenderCount() - 1),
		.size = sizeof(ParticleData) * _numParticles
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
		.dstAccessMask = vk::AccessFlagBits::eShaderRead,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.buffer = _particleData.buffer(),
		.offset = sizeof(ParticleData) * _numParticles * (_context->renderer().framePrerenderCount() - 1),
		.size = initializeBuffer.size(),
	}}, {});


	// keep alive until next use of frame sync
	_context->renderer().stage([initializeBuffer = std::move(initializeBuffer)] (auto) {});

	initialized = true;
}

}