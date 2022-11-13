#include <pbf/RadixSort.h>
#include <pbf/ContextInterface.h>
#include <pbf/Cache.h>
#include <pbf/descriptors/DescriptorSetLayout.h>
#include "pbf/descriptors/PipelineLayout.h"
#include "pbf/descriptors/ComputePipeline.h"

namespace {
auto roundToNextMultiple(auto value, auto mod) {
	if (value % mod) {
		return value + (mod - (value % mod));
	} else {
		return value;
	}
}
}

namespace pbf {

RadixSort::RadixSort(
	ContextInterface& _context,
	uint32_t _blockSize,
	uint32_t _numBlocks,
	descriptors::DescriptorSetLayout const& _keyAndGlobalSortShaderDescriptors,
	std::string_view _shaderPrefix
):
context(_context), blockSize(_blockSize), numBlocks(_numBlocks),
prefixSums(_context, numKeys, vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC)
{
	for (ssize_t blockSumSize = 4 * numBlocks; blockSumSize > 1; blockSumSize = (blockSumSize + blockSize - 1) / blockSize)
	{
		blockSums.emplace_back(
			_context,
			roundToNextMultiple(blockSumSize, blockSize),
			vk::BufferUsageFlagBits::eStorageBuffer,
			MemoryType::STATIC
		);
	}
	blockSums.emplace_back(_context, blockSize, vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC);


	auto& prescanBlockSum = blockSums.front();

	auto& cache = context.cache();

	auto sortLayout = cache.fetch(descriptors::DescriptorSetLayout{
		.createFlags = {},
		.bindings = {{
						 .binding = 1,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1,
						 .stageFlags = vk::ShaderStageFlagBits::eCompute
					 },{
						 .binding = 2,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1,
						 .stageFlags = vk::ShaderStageFlagBits::eCompute
					 },{
						 .binding = 3,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1,
						 .stageFlags = vk::ShaderStageFlagBits::eCompute
					 }},
		PBF_DESC_DEBUG_NAME("Stub Descriptor Set Layout")
	});
	auto sortPipelineLayout = cache.fetch(
		descriptors::PipelineLayout{
			.setLayouts = {sortLayout, cache.fetch(_keyAndGlobalSortShaderDescriptors)},
			.pushConstants = {
				vk::PushConstantRange{
					vk::ShaderStageFlagBits::eCompute,
					0,
					sizeof(PushConstants)
				}
			},
			PBF_DESC_DEBUG_NAME("prescan pipeline Layout")
		});
	prescanPipeline = cache.fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{fmt::format("{}_prescan.comp.spv", _shaderPrefix)},
						PBF_DESC_DEBUG_NAME("RadixSort: Prescan Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize / 2}
				}
			},
			.pipelineLayout = cache.fetch(
				descriptors::PipelineLayout{
					.setLayouts = {
						cache.fetch(
							descriptors::DescriptorSetLayout{
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
								PBF_DESC_DEBUG_NAME("Prescan Set Layout")
							}
						),
						cache.fetch(
							descriptors::DescriptorSetLayout{
								.createFlags = {},
								.bindings = {{
									.binding = 0,
									.descriptorType = vk::DescriptorType::eStorageBuffer,
									.descriptorCount = 1,
									.stageFlags = vk::ShaderStageFlagBits::eCompute
								}},
								PBF_DESC_DEBUG_NAME("Prescan Hash Particle Key Layout")
							}
						),
						cache.fetch(
							descriptors::DescriptorSetLayout{
								.createFlags = {},
								.bindings = {{
									.binding = 0,
									.descriptorType = vk::DescriptorType::eUniformBuffer,
									.descriptorCount = 1,
									.stageFlags = vk::ShaderStageFlagBits::eCompute
								}},
								PBF_DESC_DEBUG_NAME("Prescan Hash Grid Data Layout")
							}
						),
					},
					.pushConstants = {
						vk::PushConstantRange{
							vk::ShaderStageFlagBits::eCompute,
							0,
							sizeof(PushConstants)
						}
					},
					PBF_DESC_DEBUG_NAME("prescan pipeline Layout")
				}),
			PBF_DESC_DEBUG_NAME("prescan pipeline")
		}
	);
	scanPipeline = cache.fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/radixsort/scan.comp.spv"},
						PBF_DESC_DEBUG_NAME("Radix Sort: Scan Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize / 2 }
				}
			},
			.pipelineLayout = cache.fetch(
				descriptors::PipelineLayout{
					.setLayouts = {
						cache.fetch(
							descriptors::DescriptorSetLayout{
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
								PBF_DESC_DEBUG_NAME("Prescan Set Layout")
							}
						)
					},
					PBF_DESC_DEBUG_NAME("scan pipeline Layout")
				}),
			PBF_DESC_DEBUG_NAME("scan pipeline")
		}
	);

	addBlockSumPipeline = cache.fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/radixsort/addblocksum.comp.spv"},
						PBF_DESC_DEBUG_NAME("Radix Sort: Add Block Sum Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize }
				}
			},
			.pipelineLayout = cache.fetch(
				descriptors::PipelineLayout{
					.setLayouts = {
						cache.fetch(
							descriptors::DescriptorSetLayout{
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
								PBF_DESC_DEBUG_NAME("Add block sum Set Layout")
							}
						)
					},
					PBF_DESC_DEBUG_NAME("add block sum pipeline Layout")
				}),
			PBF_DESC_DEBUG_NAME("add block sum pipeline")
		}
	);


	globalSortPipeline = cache.fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{fmt::format("{}_globalsort.comp.spv", _shaderPrefix)},
						PBF_DESC_DEBUG_NAME("Radix Sort: Global Sort Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize }
				}
			},
			.pipelineLayout = cache.fetch(
				descriptors::PipelineLayout{
					.setLayouts = {
						cache.fetch(
							descriptors::DescriptorSetLayout{
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
								PBF_DESC_DEBUG_NAME("global sort Set Layout")
							}
						),
						cache.fetch(
							descriptors::DescriptorSetLayout{
								.createFlags = {},
								.bindings = {{
									.binding = 0,
									.descriptorType = vk::DescriptorType::eStorageBuffer,
									.descriptorCount = 1,
									.stageFlags = vk::ShaderStageFlagBits::eCompute
								}},
								PBF_DESC_DEBUG_NAME("global sort Hash Particle Key Layout")
							}
						),
						cache.fetch(
							descriptors::DescriptorSetLayout{
								.createFlags = {},
								.bindings = {{
									.binding = 0,
									.descriptorType = vk::DescriptorType::eUniformBuffer,
									.descriptorCount = 1,
									.stageFlags = vk::ShaderStageFlagBits::eCompute
								}},
								PBF_DESC_DEBUG_NAME("global sort Hash Grid Data Layout")
							}
						),
						cache.fetch(
							descriptors::DescriptorSetLayout{
								.createFlags = {},
								.bindings = {{
									.binding = 0,
									.descriptorType = vk::DescriptorType::eStorageBuffer,
									.descriptorCount = 1,
									.stageFlags = vk::ShaderStageFlagBits::eCompute
								}},
								PBF_DESC_DEBUG_NAME("global sort output Data Layout")
							}
						),
					},
					.pushConstants = {
						vk::PushConstantRange{
							vk::ShaderStageFlagBits::eCompute,
							0,
							sizeof(PushConstants)
						}
					},
					PBF_DESC_DEBUG_NAME("global sort pipeline Layout")
				}),
			PBF_DESC_DEBUG_NAME("global sort pipeline")
		}
	);

	prescanParams = context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = context.descriptorPool(),
		.descriptorSetCount = 1,
		.pSetLayouts = &*sortLayout
	}).front();
	{
		auto descriptorInfos = {
			vk::DescriptorBufferInfo{prefixSums.buffer(), 0, prefixSums.deviceSize()},
			vk::DescriptorBufferInfo{prescanBlockSum.buffer(), 0, prescanBlockSum.deviceSize()}
		};
		context.device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = prescanParams,
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(descriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*descriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}}, {});
	}

	std::vector sortLayouts(blockSums.size() - 1, *sortLayout);
	scanParams = context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = context.descriptorPool(),
		.descriptorSetCount = static_cast<uint32_t>(sortLayouts.size()),
		.pSetLayouts = sortLayouts.data()
	});
	for (size_t i = 0; i < blockSums.size() - 1; ++i)
	{
		auto descriptorInfos = {
			vk::DescriptorBufferInfo{blockSums[i].buffer(), 0, blockSums[i].deviceSize()},
			vk::DescriptorBufferInfo{blockSums[i + 1].buffer(), 0, blockSums[i + 1].deviceSize()}
		};
		context.device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = scanParams[i],
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(descriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*descriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}}, {});
	}

	globalSortParams = context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = context.descriptorPool(),
		.descriptorSetCount = 1,
		.pSetLayouts = &*sortLayout
	}).front();
	{
		auto descriptorInfos = {
			vk::DescriptorBufferInfo{prefixSums.buffer(), 0, prefixSums.deviceSize()},
			vk::DescriptorBufferInfo{prescanBlockSum.buffer(), 0, prescanBlockSum.deviceSize()},
		};
		context.device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = globalSortParams,
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(descriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*descriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}}, {});
	}
}

RadixSort::Result RadixSort::stage(
	vk::CommandBuffer buf,
	uint32_t _sortBits,
	vk::DescriptorBufferInfo _initBufferInfo,
	vk::DescriptorBufferInfo _pingBufferInfo,
	vk::DescriptorBufferInfo _pongBufferInfo,
	vk::DescriptorBufferInfo _gridDataBufferInfo
) const
{
	bool isSwapped = false;

	PushConstants pushConstants {
		.blockSumOffset = glm::u32vec4{
			0, numBlocks, numBlocks * 2, numBlocks * 3
		},
		.bit = 0
	};

	for (uint32_t bit = 0; bit < _sortBits; bit += 2) {
		pushConstants.bit = bit;

		auto& prescanBlockSum = blockSums.front();
		context.bindPipeline(
			buf,
			prescanPipeline,
			{
				{ // set 0
					prefixSums.fullBufferInfo(),
					prescanBlockSum.fullBufferInfo()
				},
				{ // set 1
					(bit == 0) ? _initBufferInfo : _pingBufferInfo
				},
				{ // set 2
					// grid stuff
					_gridDataBufferInfo
				}
			}
		);

		buf.pushConstants(*(prescanPipeline.descriptor().pipelineLayout), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushConstants), &pushConstants);
		// reads keys; writes prefix sums and block sums
		buf.dispatch(((numKeys + blockSize - 1) / blockSize), 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead
			}
		}, {}, {});

		for (size_t i = 0; i < blockSums.size() - 1; ++i)
		{
			auto& prefixSum = blockSums[i];
			auto& blockSum = blockSums[i + 1];
			const uint32_t numBlockSums = (prefixSum.size() + blockSize - 1) / blockSize;

			context.bindPipeline(
				buf,
				scanPipeline,
				{ // set 0
					{prefixSum.fullBufferInfo(), blockSum.fullBufferInfo()}
				}
			);
			buf.dispatch(numBlockSums, 1, 1);

			buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
					.dstAccessMask = vk::AccessFlagBits::eShaderRead
				}
			}, {}, {});
		}

		for (ssize_t i = blockSums.size() - 2; i > 0; i--) {
			auto& prefixSum = blockSums[i - 1];
			auto& blockSum = blockSums[i];
			const uint32_t numBlockSums = (prefixSum.size() + blockSize - 1) / blockSize;

			context.bindPipeline(
				buf,
				addBlockSumPipeline,
				{ // set 0
					{prefixSum.fullBufferInfo(), blockSum.fullBufferInfo()}
				}
			);
			buf.dispatch(numBlockSums, 1, 1);

			buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
					.dstAccessMask = vk::AccessFlagBits::eShaderRead
				}
			}, {}, {});
		}

		context.bindPipeline(
			buf,
			globalSortPipeline,
			{
				{ // set 0
					prefixSums.fullBufferInfo(),
					prescanBlockSum.fullBufferInfo()
				},
				{ // set 1
					{(bit == 0) ? _initBufferInfo: _pingBufferInfo}
				},
				{ // set 2
					// grid stuff
					_gridDataBufferInfo
				},
				{ // set 3
					// grid stuff
					_pongBufferInfo
				}
			}
		);
		buf.pushConstants(*(globalSortPipeline.descriptor().pipelineLayout), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushConstants), &pushConstants);
		// reads keys, prefix sums and block sums; writes result
		buf.dispatch(((numKeys + blockSize - 1) / blockSize), 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead
			}
		}, {}, {});

		std::swap(_pingBufferInfo, _pongBufferInfo);
		isSwapped = !isSwapped;
	} // END OF BIT LOOP

	return isSwapped ? Result::InPongBuffer : Result::InPingBuffer;
}

}