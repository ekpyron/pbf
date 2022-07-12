#include "Simulation.h"
#include "Context.h"
#include "Renderer.h"

namespace pbf {

Simulation::Simulation(Context *context, size_t numParticles, vk::DescriptorBufferInfo input, vk::DescriptorBufferInfo output):
_context(context), _numParticles(numParticles), _input(input), _output(output)
{
	if (getNumParticles() % blockSize)
		throw std::runtime_error("Particle count must be a multiple of blockSize.");
	if (!getNumParticles())
		throw std::runtime_error("Need at least block size particles.");

	_particleSortKeys = Buffer{
		context,
		32 * _numParticles * context->renderer().framePrerenderCount(),
		vk::BufferUsageFlagBits::eStorageBuffer,
		pbf::MemoryType::STATIC
	};

	std::uint32_t numBlocks = getNumParticles() / blockSize;
	std::uint32_t numBlockSums = numBlocks;
	_scanStages.resize(1 + static_cast<int>(ceil (log (((numBlockSums + blockSize - 1) / blockSize) * blockSize) / log (blockSize))));

	assert(!_scanStages.empty());

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


	_scanParamsLayout = context->cache().fetch(descriptors::DescriptorSetLayout{
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
		,.debugName = "Scan stage Set Layout"
#endif
	});

	_scanPipeline = _context->cache().fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = _context->cache().fetch(
					pbf::descriptors::ShaderModule{
						.filename = "shaders/sort/scan.comp.spv",
						PBF_DESC_DEBUG_NAME("shaders/sort/scan.comp.spv Compute Shader")
					}),
				.entryPoint = "main"
			},
			.pipelineLayout = _context->cache().fetch(
				pbf::descriptors::PipelineLayout{
					.setLayouts = {{
						_scanParamsLayout
					}},
					PBF_DESC_DEBUG_NAME("scan pipeline Layout")
				}),
			PBF_DESC_DEBUG_NAME("scan pipeline")
		}
	);

	for (auto& scanStage: _scanStages) {
		scanStage.buffer = Buffer{
			context,
			numBlockSums > 0 ? numBlockSums * 4u : 4u,
			vk::BufferUsageFlagBits::eStorageBuffer,
			MemoryType::STATIC
		};


		scanStage.params = context->device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = *_descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &*_scanParamsLayout
		}).front();

		numBlockSums /= blockSize;
	}

	{
		std::array<vk::DescriptorBufferInfo, 2> descriptorInfos {{
			{_particleSortKeys.buffer(), 0, _particleSortKeys.size()},
			{_scanStages.front().buffer.buffer(), 0, _scanStages.front().buffer.size()}
		}};

		context->device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = _scanStages.front().params,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = descriptorInfos.size(),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = descriptorInfos.data(),
			.pTexelBufferView = nullptr
		}}, {});
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
					 },
					 {
						 .binding = 3,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1, // TODO: check if this can be = 3
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
		std::array<vk::DescriptorBufferInfo, 4> descriptorInfos {{input, output, vk::DescriptorBufferInfo{
			_particleSortKeys.buffer(),
			0,
			VK_WHOLE_SIZE
		}, vk::DescriptorBufferInfo{
			_scanStages.front().buffer.buffer(),
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

	_prescanPipeline = _context->cache().fetch(
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
					PBF_DESC_DEBUG_NAME("prescan pipeline Layout")
				}),
			PBF_DESC_DEBUG_NAME("prescan pipeline")
		}
	);

	for (size_t i = 0u; i < _scanStages.size() - 1; ++i) {
		auto& scanStage = _scanStages[i];
		auto& nextScanStage = _scanStages[i + 1];

		std::array<vk::DescriptorBufferInfo, 2> descriptorInfos {{
			{scanStage.buffer.buffer(), 0, scanStage.buffer.size()},
			{nextScanStage.buffer.buffer(), 0, nextScanStage.buffer.size()}
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
	_prescanPipeline.keepAlive();

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


	buf.bindPipeline(vk::PipelineBindPoint::eCompute, *_prescanPipeline);
	buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(_prescanPipeline.descriptor().pipelineLayout), 0, { _prescanParams }, {});
	std::uint32_t numGroups = (getNumParticles() + 255)/256;
	buf.dispatch(numGroups, 1, 1);

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eComputeShader, {}, {}, {vk::BufferMemoryBarrier{
		.srcAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eMemoryWrite,
		.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead|vk::AccessFlagBits::eShaderRead,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.buffer = _output.buffer,
		.offset = 0,
		.size = _output.range
	}}, {});

	// particleKeys in prescan: 262144
	// prescanWrites 1024 blocksums
	// scanStage[0] writes 4 blocksums
	// scanStage[1] writes 1 blocksum

	buf.bindPipeline(vk::PipelineBindPoint::eCompute, *_scanPipeline);
	// for each stage
	for (size_t i = 0u; i < _scanStages.size() - 1; ++i) {
		auto& scanStage = _scanStages.at(i);
		buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(_scanPipeline.descriptor().pipelineLayout), 0, { scanStage.params }, {});
		numGroups = (numGroups + 255) / 256;
		buf.dispatch(numGroups, 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {vk::BufferMemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eMemoryWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.buffer = scanStage.buffer.buffer(),
			.offset = 0,
			.size = scanStage.buffer.size()
		}}, {});

	}

//	buffer->end();

//	return buffer;
}

void Simulation::initialize(vk::CommandBuffer buf)
{

	initialized = true;
}

}