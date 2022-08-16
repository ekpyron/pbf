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
	Cache& _cache,
	uint32_t _blockSize,
	uint32_t _numBlocks,
	descriptors::DescriptorSetLayout const& _keyAndGlobalSortShaderDescriptors,
	descriptors::ShaderModule const& _keyShaderModule,
	descriptors::ShaderModule const& _globalSortShaderModule
):
context(_context), cache(_cache), blockSize(_blockSize), numBlocks(_numBlocks),
prefixSums(&_context, numKeys, vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC)
{
	for (ssize_t blockSumSize = 4 * numBlocks; blockSumSize > 1; blockSumSize = (blockSumSize + blockSize - 1) / blockSize)
	{
		blockSums.emplace_back(
			&_context,
			roundToNextMultiple(blockSumSize, blockSize),
			vk::BufferUsageFlagBits::eStorageBuffer,
			MemoryType::STATIC
		);
	}
	blockSums.emplace_back(&_context, blockSize, vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC);


	auto& prescanBlockSum = blockSums.front();

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
	auto prescanPipeline = cache.fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/radixsort/prescan.comp.spv"},
						PBF_DESC_DEBUG_NAME("RadixSort: Prescan Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize / 2}
				}
			},
			.pipelineLayout = sortPipelineLayout,
			PBF_DESC_DEBUG_NAME("prescan pipeline")
		}
	);
	auto scanPipeline = cache.fetch(
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
			.pipelineLayout = sortPipelineLayout,
			PBF_DESC_DEBUG_NAME("scan pipeline")
		}
	);

	auto addBlockSumPipeline = cache.fetch(
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
			.pipelineLayout = sortPipelineLayout,
			PBF_DESC_DEBUG_NAME("add block sum pipeline")
		}
	);


	auto globalSortPipeline = cache.fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/radixsort/globalsort.comp.spv"},
						PBF_DESC_DEBUG_NAME("Radix Sort: Global Sort Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize }
				}
			},
			.pipelineLayout = sortPipelineLayout,
			PBF_DESC_DEBUG_NAME("global sort pipeline")
		}
	);
	vk::UniqueDescriptorPool descriptorPool;
	{
		std::uint32_t numDescriptorSets = 1024;
		static std::array<vk::DescriptorPoolSize, 1> sizes {{{ vk::DescriptorType::eStorageBuffer, 1024 }}};
		descriptorPool = context.device().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
			.maxSets = numDescriptorSets,
			.poolSizeCount = static_cast<std::uint32_t>(sizes.size()),
			.pPoolSizes = sizes.data()
		});
	}

	auto prescanParams = context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = *descriptorPool,
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
	auto scanParams = context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = *descriptorPool,
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

	auto globalSortParams = context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = *descriptorPool,
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

}