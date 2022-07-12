#include "Simulation.h"
#include "Context.h"
#include "Renderer.h"
#include <pbf/descriptors/ComputePipeline.h>

namespace pbf {

Simulation::Simulation(Context *context, size_t numParticles, vk::DescriptorBufferInfo input, vk::DescriptorBufferInfo output):
_context(context), _numParticles(numParticles), _input(input), _output(output)
{
	_particleSortKeys = Buffer{
		context,
		32 * _numParticles * context->renderer().framePrerenderCount(),
		vk::BufferUsageFlagBits::eStorageBuffer,
		pbf::MemoryType::STATIC
	};

	std::uint32_t numBlocks = getNumParticles() / blockSize;
	std::uint32_t numBlockSums = numBlocks;
	_scanStages.resize(static_cast<int>(ceil (log (((numBlockSums + blockSize - 1) / blockSize) * blockSize) / log (blockSize))));

	{
		std::uint32_t numDescriptorSets = 1 + _scanStages.size();
		static std::array<vk::DescriptorPoolSize, 1> sizes {{
			{ vk::DescriptorType::eStorageBuffer, 2 }
		}};
		_descriptorPool = context->device().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
			.maxSets = numDescriptorSets,
			.poolSizeCount = static_cast<std::uint32_t>(sizes.size()),
			.pPoolSizes = sizes.data()
		});
	}

	_prescanParamsLayout = context->cache().fetch(descriptors::DescriptorSetLayout{
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
					 },
					 {
						 .binding = 2,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1,
						 .stageFlags = vk::ShaderStageFlagBits::eCompute
					 }}
#ifndef NDEBUG
		,.debugName = "Simulation Descriptor Set Layout"
#endif
	});
	_prescanParams = context->device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = *_descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &*_prescanParamsLayout
	}).front();
	{
		std::array<vk::DescriptorBufferInfo, 3> descriptorInfos {{input, output, vk::DescriptorBufferInfo{
			_particleSortKeys.buffer(),
			0,
			VK_WHOLE_SIZE
		}}};
		context->device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = _prescanParams,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = descriptorInfos.size(),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = descriptorInfos.data(),
			.pTexelBufferView = nullptr
		}}, {});
	}


	if (getNumParticles() % blockSize)
		throw std::runtime_error("Particle count must be a multiple of blockSize.");

	for (
#ifndef NDEBUG
		size_t i = 0u;
#endif
		auto& scanStage: _scanStages
	) {
		scanStage.buffer = Buffer{
			context,
			numBlockSums * 4u,
			vk::BufferUsageFlagBits::eStorageBuffer,
			MemoryType::STATIC
		};

		scanStage.paramsLayout = context->cache().fetch(descriptors::DescriptorSetLayout{
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
			,.debugName = fmt::format("Scan stage Set Layout {}", i++)
#endif
		});

		scanStage.params = context->device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = *_descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &*scanStage.paramsLayout
		}).front();

		numBlockSums /= blockSize;
	}

	for (size_t i = 1u; i < _scanStages.size(); ++i) {
		auto& previousScanStage = _scanStages[i - 1];
		auto& scanStage = _scanStages[i];

		std::array<vk::DescriptorBufferInfo, 2> descriptorInfos {{
			{previousScanStage.buffer.buffer(), 0, previousScanStage.buffer.size()},
			{scanStage.buffer.buffer(), 0, scanStage.buffer.size()}
		}};

		context->device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = scanStage.params,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = descriptorInfos.size(),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = descriptorInfos.data(),
			.pTexelBufferView = nullptr
		}}, {});



	}
}

void Simulation::run(vk::CommandBuffer buf)
{
	if (!initialized)
		initialize(buf);

	_prescanParamsLayout.keepAlive();
	const auto& computePipeline = _context->cache().fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = _context->cache().fetch(
					pbf::descriptors::ShaderModule{
						.filename = "shaders/sort/prescan.comp.spv",
						PBF_DESC_DEBUG_NAME("shaders/sort/prescan.comp.spv Compute Shader")
					}),
				.entryPoint = "main"
			},
			.pipelineLayout = _context->cache().fetch(
				pbf::descriptors::PipelineLayout{
					.setLayouts = {{
						_prescanParamsLayout
					}},
					.pushConstants = {vk::PushConstantRange{
						.stageFlags = vk::ShaderStageFlagBits::eCompute,
						.offset = 0,
						.size = sizeof(PushConstantData)
					}},
					PBF_DESC_DEBUG_NAME("Simulation Pipeline Layout")
				}),
			PBF_DESC_DEBUG_NAME("sort pipeline layout")
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
	buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(computePipeline.descriptor().pipelineLayout), 0, { _prescanParams }, {});
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
		.buffer = _output.buffer,
		.offset = 0,
		.size = _output.range
	}}, {});

//	buffer->end();

//	return buffer;
}

void Simulation::initialize(vk::CommandBuffer buf)
{

	initialized = true;
}

}