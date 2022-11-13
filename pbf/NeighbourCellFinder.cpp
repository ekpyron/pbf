#include <pbf/NeighbourCellFinder.h>
#include <pbf/Context.h>
#include <pbf/Buffer.h>

namespace pbf {


NeighbourCellFinder::NeighbourCellFinder(ContextInterface& context, size_t numGridCells, size_t maxID):
_context(context),
_gridBoundaryBuffer(context, numGridCells, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst, MemoryType::STATIC)
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
					Specialization<uint32_t>{.constantID = 0, .value = blockSize},
                    Specialization<uint32_t>{.constantID = 1, .value = static_cast<uint32_t>(maxID)}
				}
			},
			.pipelineLayout = neighbourCellPipelineLayout,
			PBF_DESC_DEBUG_NAME("NeighbourCellFinder: find cells pipeline")
		}
	);
}

void NeighbourCellFinder::operator()(vk::CommandBuffer buf, uint32_t numParticles, vk::DescriptorBufferInfo const& input, vk::DescriptorBufferInfo const& gridData) const
{
	buf.fillBuffer(_gridBoundaryBuffer.buffer(), 0, _gridBoundaryBuffer.deviceSize(), 0);

	_context.bindPipeline(
		buf,
		_findCellsPipeline,
		{
			{input, gridData},
			{_gridBoundaryBuffer.fullBufferInfo()},
		}
	);

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