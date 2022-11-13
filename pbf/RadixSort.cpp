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
	std::vector<descriptors::DescriptorSetLayout> const& _keyAndGlobalSortShaderDescriptorLayouts,
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

	std::vector<CacheReference<descriptors::DescriptorSetLayout>> setLayouts{
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
		)
	};
	for (auto const& layoutDescriptor: _keyAndGlobalSortShaderDescriptorLayouts)
		setLayouts.emplace_back(cache.fetch(layoutDescriptor));

	auto prescanAndGlobalortPipelineLayout = cache.fetch(
	descriptors::PipelineLayout{
		.setLayouts = setLayouts,
		.pushConstants = {
			vk::PushConstantRange{
				vk::ShaderStageFlagBits::eCompute,
				0,
				sizeof(PushConstants)
			}
		},
		PBF_DESC_DEBUG_NAME("prescan and global sort pipeline Layout")
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
			.pipelineLayout = prescanAndGlobalortPipelineLayout,
			PBF_DESC_DEBUG_NAME("prescan pipeline")
		}
	);
	auto scanAndAddBlockSumLayout = cache.fetch(descriptors::PipelineLayout{
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
					PBF_DESC_DEBUG_NAME("scan and add block sum set Layout")
				}
			)
		},
		PBF_DESC_DEBUG_NAME("scan and add block sum pipeline Layout")
	});
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
			.pipelineLayout = scanAndAddBlockSumLayout,
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
			.pipelineLayout = scanAndAddBlockSumLayout,
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
			.pipelineLayout = prescanAndGlobalortPipelineLayout,
			PBF_DESC_DEBUG_NAME("global sort pipeline")
		}
	);
}

RadixSort::Result RadixSort::stage(
	vk::CommandBuffer buf,
	uint32_t _sortBits,
	std::vector<descriptors::DescriptorSetBinding> initInfos,
	std::vector<descriptors::DescriptorSetBinding> pingInfos,
	std::vector<descriptors::DescriptorSetBinding> pongInfos
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
				(bit == 0) ? initInfos : pingInfos
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
				(bit == 0) ? initInfos : pingInfos
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

		std::swap(pingInfos, pongInfos);
		isSwapped = !isSwapped;
	} // END OF BIT LOOP

	return isSwapped ? Result::InPongBuffer : Result::InPingBuffer;
}

}