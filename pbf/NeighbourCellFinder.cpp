#include <pbf/NeighbourCellFinder.h>
#include <pbf/Context.h>
#include <pbf/Buffer.h>

namespace pbf {


NeighbourCellFinder::NeighbourCellFinder(ContextInterface& context, size_t numGridCells):
_gridBoundaryBuffer(context, numGridCells, vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC)
{
	auto& cache = context.cache();

	auto inputDescriptorSetLayout = cache.fetch(descriptors::DescriptorSetLayout{
		.createFlags = {},
		.bindings = {{
						 .binding = 0,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1,
						 .stageFlags = vk::ShaderStageFlagBits::eCompute
					 },{
						 .binding = 1,
						 .descriptorType = vk::DescriptorType::eUniformBuffer,
						 .descriptorCount = 1,
						 .stageFlags = vk::ShaderStageFlagBits::eCompute
					 }},
		PBF_DESC_DEBUG_NAME("Neighbour Cell Input Descriptor Layout")
	});
	auto gridDataDescriptorSetLayout = cache.fetch(descriptors::DescriptorSetLayout{
		.createFlags = {},
		.bindings = {{
						 .binding = 0,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1,
						 .stageFlags = vk::ShaderStageFlagBits::eCompute
					 }},
		PBF_DESC_DEBUG_NAME("Neighbour Cell Grid Data Layout")
	});
	auto neighbourCellPipelineLayout = cache.fetch(
		descriptors::PipelineLayout{
			.setLayouts = {inputDescriptorSetLayout, gridDataDescriptorSetLayout},
			PBF_DESC_DEBUG_NAME("Neighbour cell pipeline Layout")
		});

	_findCellsPipeline = cache.fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/neighbour/findcells.comp.spv"},
						PBF_DESC_DEBUG_NAME("NeighbourCellFinder: Find Cells Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize}
				}
			},
			.pipelineLayout = neighbourCellPipelineLayout,
			PBF_DESC_DEBUG_NAME("NeighbourCellFinder: find cells pipeline")
		}
	);


	_testPipeline = cache.fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/neighbour/test.comp.spv"},
						PBF_DESC_DEBUG_NAME("NeighbourCellFinder: Test Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize}
				}
			},
			.pipelineLayout = neighbourCellPipelineLayout,
			PBF_DESC_DEBUG_NAME("NeighbourCellFinder: test pipeline")
		}
	);
	_gridData = context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = context.descriptorPool(),
		.descriptorSetCount = 1,
		.pSetLayouts = &*gridDataDescriptorSetLayout
	}).front();
	{
		auto descriptorInfos = {
			vk::DescriptorBufferInfo{_gridBoundaryBuffer.buffer(), 0, _gridBoundaryBuffer.deviceSize()},
		};
		context.device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = _gridData,
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

void NeighbourCellFinder::operator()(vk::CommandBuffer buf, uint32_t numParticles, vk::DescriptorSet input) const
{
	buf.bindPipeline(vk::PipelineBindPoint::eCompute, *_findCellsPipeline);
	buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(_findCellsPipeline.descriptor().pipelineLayout), 0, {input, _gridData}, {});
	// reads keys; writes prefix sums and block sums
	buf.dispatch(((numParticles + blockSize - 1) / blockSize), 1, 1);

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
		vk::MemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead
		}
	}, {}, {});

	buf.bindPipeline(vk::PipelineBindPoint::eCompute, *_testPipeline);
	buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(_findCellsPipeline.descriptor().pipelineLayout), 0, {input, _gridData}, {});
	// reads keys; writes prefix sums and block sums
	buf.dispatch(((numParticles + blockSize - 1) / blockSize), 1, 1);

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
		vk::MemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead
		}
	}, {}, {});

}

}