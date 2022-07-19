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
		4 * _numParticles,
		vk::BufferUsageFlagBits::eStorageBuffer,
		pbf::MemoryType::DYNAMIC
	};

	std::uint32_t numBlocks = getNumParticles() / blockSize;
	std::uint32_t numBlockSums = numBlocks;
	_scanStages.resize(1 + static_cast<int>(ceil (log (((numBlockSums + blockSize - 1) / blockSize) * blockSize) / log (blockSize))));

	assert(!_scanStages.empty());

	{
		std::uint32_t numDescriptorSets = 512; // 1 + _scanStages.size();
		static std::array<vk::DescriptorPoolSize, 1> sizes {{
			{ vk::DescriptorType::eStorageBuffer, 1024 }
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
					 }},
		PBF_DESC_DEBUG_NAME("Scan stage Set Layout")
	});

	_scanPipeline = _context->cache().fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = _context->cache().fetch(
					pbf::descriptors::ShaderModule{
						.source = pbf::descriptors::ShaderModule::File{"shaders/sort/scan.comp.spv"},
						PBF_DESC_DEBUG_NAME("shaders/sort/scan.comp.spv Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t> { .constantID = 0, .value = blockSize / 2 }
				}
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
			MemoryType::DYNAMIC
		};


		scanStage.params = context->device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = *_descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &*_scanParamsLayout
		}).front();

		numBlockSums += blockSize - 1;
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
					 }},
		PBF_DESC_DEBUG_NAME("Simulation Prescan Descriptor Set Layout")
	});
	_prescanParams = context->device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = *_descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &*_prescanParamsLayout
	}).front();
	{
		std::array<vk::DescriptorBufferInfo, 4> descriptorInfos {{
			vk::DescriptorBufferInfo{
				_particleSortKeys.buffer(),
				0,
				VK_WHOLE_SIZE
			},
			vk::DescriptorBufferInfo{
				_scanStages.front().buffer.buffer(),
				0,
				VK_WHOLE_SIZE
			},
			input,
			output,
		}};
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
						.source = pbf::descriptors::ShaderModule::File{"shaders/sort/prescan.comp.spv"},
						PBF_DESC_DEBUG_NAME("shaders/sort/prescan.comp.spv Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t> { .constantID = 0, .value = blockSize / 2 },
					Specialization<uint32_t> { .constantID = 1, .value = 0 }
				}
			},
			.pipelineLayout = _context->cache().fetch(
				pbf::descriptors::PipelineLayout{
					.setLayouts = {{
						_prescanParamsLayout
					}},
					.pushConstants = {
						vk::PushConstantRange{
							.stageFlags = vk::ShaderStageFlagBits::eCompute,
							.offset = 0,
							.size = sizeof(uint32_t)
						}
					},
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

	_assignParamsLayout = context->cache().fetch(descriptors::DescriptorSetLayout{
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
					 }},
		PBF_DESC_DEBUG_NAME("Simulation Assign Descriptor Set Layout")
	});
	_assignParams = context->device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = *_descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &*_assignParamsLayout
	}).front();
	{
		auto descriptorInfos = {
			vk::DescriptorBufferInfo{
				_particleSortKeys.buffer(),
				0,
				VK_WHOLE_SIZE
			},
			input,
			output,
		};
		context->device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = _assignParams,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(descriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*descriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}}, {});
	}
	_assignPipeline = _context->cache().fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = _context->cache().fetch(
					pbf::descriptors::ShaderModule{
						.source = pbf::descriptors::ShaderModule::File{"shaders/sort/assign.comp.spv"},
						PBF_DESC_DEBUG_NAME("shaders/sort/assign.comp.spv Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t> { .constantID = 0, .value = blockSize }
				}
			},
			.pipelineLayout = _context->cache().fetch(
				pbf::descriptors::PipelineLayout{
					.setLayouts = {{
						_assignParamsLayout
					}},
					.pushConstants = {
						vk::PushConstantRange{
							.stageFlags = vk::ShaderStageFlagBits::eCompute,
							.offset = 0,
							.size = sizeof(uint32_t)
						}
					},
					PBF_DESC_DEBUG_NAME("assign pipeline Layout")
				}),
			PBF_DESC_DEBUG_NAME("assign pipeline")
		}
	);




	_addBlockSumsParamsLayout = context->cache().fetch(descriptors::DescriptorSetLayout{
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
					 }},
		PBF_DESC_DEBUG_NAME("Simulation Add Block Sums Descriptor Set Layout")
	});
	for (size_t i = 1u; i < _scanStages.size(); ++i)
	{
		auto& previousStage = _scanStages[i - 1];
		auto& stage = _scanStages[i];

		stage.addBlockSumsParams = context->device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = *_descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &*_addBlockSumsParamsLayout
		}).front();
		{
			auto descriptorInfos = {
				vk::DescriptorBufferInfo{
					previousStage.buffer.buffer(),
					0,
					VK_WHOLE_SIZE
				},
				vk::DescriptorBufferInfo{
					stage.buffer.buffer(),
					0,
					VK_WHOLE_SIZE
				}
			};
			context->device().updateDescriptorSets({vk::WriteDescriptorSet{
				.dstSet = stage.addBlockSumsParams,
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = static_cast<uint32_t>(descriptorInfos.size()),
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.pImageInfo = nullptr,
				.pBufferInfo = &*descriptorInfos.begin(),
				.pTexelBufferView = nullptr
			}}, {});
		}
	}
	// TODO: combine with above
	{
		auto& stage = _scanStages.front();

		stage.addBlockSumsParams = context->device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = *_descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &*_addBlockSumsParamsLayout
		}).front();
		{
			std::array<vk::DescriptorBufferInfo, 2> descriptorInfos{{
				vk::DescriptorBufferInfo{
					_particleSortKeys.buffer(),
					0,
					_particleSortKeys.size()
				},
				vk::DescriptorBufferInfo{
					stage.buffer.buffer(),
					0,
					stage.buffer.size()
				}
			}};
			context->device().updateDescriptorSets({vk::WriteDescriptorSet{
				.dstSet = stage.addBlockSumsParams,
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = static_cast<uint32_t>(descriptorInfos.size()),
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.pImageInfo = nullptr,
				.pBufferInfo = descriptorInfos.data(),
				.pTexelBufferView = nullptr
			}}, {});
		}
	}

	_addBlockSumsPipeline = _context->cache().fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = _context->cache().fetch(
					pbf::descriptors::ShaderModule{
						.source = pbf::descriptors::ShaderModule::File{"shaders/sort/addblocksum.comp.spv"},
						PBF_DESC_DEBUG_NAME("shaders/sort/addblocksum.comp.spv Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t> { .constantID = 0, .value = blockSize }
				}
			},
			.pipelineLayout = _context->cache().fetch(
				pbf::descriptors::PipelineLayout{
					.setLayouts = {{
						_addBlockSumsParamsLayout
					}},
					PBF_DESC_DEBUG_NAME("add block sums pipeline Layout")
				}),
			PBF_DESC_DEBUG_NAME("add block sums pipeline")
		}
	);
}

void Simulation::run(vk::CommandBuffer buf)
{
	if (!initialized)
		initialize(buf);

	for (uint32_t bit = 0; bit < 1; ++bit)
	{

	buf.bindPipeline(vk::PipelineBindPoint::eCompute, *_prescanPipeline);
	buf.pushConstants(*(_prescanPipeline.descriptor().pipelineLayout), vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t), &bit);
	buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(_prescanPipeline.descriptor().pipelineLayout), 0, { _prescanParams }, {});
	std::uint32_t numGroups = (getNumParticles() + blockSize - 1)/blockSize;
	buf.dispatch(numGroups * 2, 1, 1);

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {vk::BufferMemoryBarrier{
		.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
		.dstAccessMask = vk::AccessFlagBits::eShaderRead,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.buffer = _output.buffer,
		.offset = _output.offset,
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
		numGroups = (numGroups + blockSize - 1) / blockSize;
		buf.dispatch(numGroups * 2, 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {vk::BufferMemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.buffer = scanStage.buffer.buffer(),
			.offset = 0,
			.size = scanStage.buffer.size()
		}}, {});
	}

	buf.bindPipeline(vk::PipelineBindPoint::eCompute, *_addBlockSumsPipeline);
	for (ssize_t i = _scanStages.size() - 2; i >= 0; --i)
	{
		auto& previousStageBuffer = (i > 0) ? _scanStages.at(i - 1).buffer : _particleSortKeys;
		auto& scanStage = _scanStages.at(i);
		buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(_addBlockSumsPipeline.descriptor().pipelineLayout), 0, { scanStage.addBlockSumsParams }, {});
		buf.dispatch((previousStageBuffer.size() / 4 + blockSize - 1) / blockSize, 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {vk::BufferMemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.buffer = previousStageBuffer.buffer(),
			.offset = 0,
			.size = previousStageBuffer.size()
		}}, {});
	}

	buf.bindPipeline(vk::PipelineBindPoint::eCompute, *_assignPipeline);
	buf.pushConstants(*(_assignPipeline.descriptor().pipelineLayout), vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t), &bit);
	buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(_assignPipeline.descriptor().pipelineLayout), 0, { _assignParams }, {});
	{
		std::uint32_t numGroups = (getNumParticles() + blockSize - 1)/blockSize;
		buf.dispatch(numGroups, 1, 1);
	}

	// TODO: adjust per bit
	buf.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader,
		vk::PipelineStageFlagBits::eVertexInput|vk::PipelineStageFlagBits::eComputeShader,
		{},
		{},
		{vk::BufferMemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
			.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead|vk::AccessFlagBits::eShaderRead,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.buffer = _output.buffer,
			.offset = _output.offset,
			.size = _output.range
		}}, {});

	}
/*
	buf.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader,
		vk::PipelineStageFlagBits::eTransfer,
		{},
		{},
		{vk::BufferMemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
			.dstAccessMask = vk::AccessFlagBits::eTransferRead,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.buffer = _input.buffer,
			.offset = _input.offset,
			.size = _input.range
		}}, {});

	buf.copyBuffer(_input.buffer, _output.buffer, {
		vk::BufferCopy{
			.srcOffset = _input.offset,
			.dstOffset = _output.offset,
			.size = _input.range
		}
	});

	buf.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eVertexInput,
		{},
		{},
		{vk::BufferMemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
			.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.buffer = _output.buffer,
			.offset = _output.offset,
			.size = _output.range
		}}, {});
*/

	{
		uint32_t *data = reinterpret_cast<uint32_t*>(_particleSortKeys.data());
		std::stringstream stream;
		for (size_t i = 0; i < getNumParticles(); ++i)
			stream << "[" << i << "]: " << data[i] << "; ";
		spdlog::get("console")->debug("particleSortKeys: {}", stream.str());
	}
	for (size_t i = 0; i < _scanStages.size(); ++i)
	{
		uint32_t *data = reinterpret_cast<uint32_t*>(_scanStages[i].buffer.data());
		std::stringstream stream;
		for (size_t j = 0; j < _scanStages[i].buffer.size() / 4; ++j)
			stream << "[" << j << "]: " << data[j] << "; ";
		spdlog::get("console")->debug("scanStage({}): {}", i, stream.str());
	}
}

void Simulation::initialize(vk::CommandBuffer buf)
{

	initialized = true;
}

}