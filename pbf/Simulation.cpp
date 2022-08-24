#include "Simulation.h"
#include "Context.h"
#include "Renderer.h"

namespace pbf {

namespace {
constexpr auto radixSortDescriptorSetLayout() {
	return descriptors::DescriptorSetLayout{
		.createFlags = {},
		.bindings = {
			{
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
				.descriptorType = vk::DescriptorType::eUniformBuffer,
				.descriptorCount = 1,
				.stageFlags = vk::ShaderStageFlagBits::eCompute
			}
		},
		PBF_DESC_DEBUG_NAME("Simulation particle sort descriptor set layout")
	};
}

}

Simulation::Simulation(InitContext &initContext, size_t numParticles, vk::DescriptorBufferInfo input, vk::DescriptorBufferInfo output):
_context(initContext.context),
_numParticles(numParticles),
_radixSort(_context, blockSize, getNumParticles() / blockSize, radixSortDescriptorSetLayout(), "shaders/particlesort"),
gridDataBuffer(_context, 1, vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst, MemoryType::STATIC),
_input(input),
_output(output)
{
	if (getNumParticles() % blockSize)
		throw std::runtime_error("Particle count must be a multiple of blockSize.");
	if (!getNumParticles())
		throw std::runtime_error("Need at least block size particles.");

	{
		std::uint32_t numDescriptorSets = 512;
		static std::array<vk::DescriptorPoolSize, 1> sizes {{
			{ vk::DescriptorType::eStorageBuffer, 1024 }
		}};
		_descriptorPool = _context.device().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
			.maxSets = numDescriptorSets,
			.poolSizeCount = static_cast<std::uint32_t>(sizes.size()),
			.pPoolSizes = sizes.data()
		});
	}


	{
		std::vector pingPongLayouts(2, *_context.cache().fetch(radixSortDescriptorSetLayout()));
		auto descriptorSets = _context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = *_descriptorPool,
			.descriptorSetCount = size32(pingPongLayouts),
			.pSetLayouts = pingPongLayouts.data()
		});
		pingDescriptorSet = descriptorSets.front();
		pongDescriptorSet = descriptorSets.back();
	}

	{
		vk::DescriptorBufferInfo gridDataBufferInfo{
			.buffer = gridDataBuffer.buffer(),
			.offset = 0u,
			.range = sizeof(GridData)
		};
		{
			auto pingDescriptorInfos = {
				input,
				output
			};
			auto pongDescriptorInfos = {
				output,
				input
			};
			_context.device().updateDescriptorSets({vk::WriteDescriptorSet{
				.dstSet = pingDescriptorSet,
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = static_cast<uint32_t>(pingDescriptorInfos.size()),
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.pImageInfo = nullptr,
				.pBufferInfo = &*pingDescriptorInfos.begin(),
				.pTexelBufferView = nullptr
			}, vk::WriteDescriptorSet{
				.dstSet = pingDescriptorSet,
				.dstBinding = 2,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eUniformBuffer,
				.pImageInfo = nullptr,
				.pBufferInfo = &gridDataBufferInfo,
				.pTexelBufferView = nullptr
			}, vk::WriteDescriptorSet{
				.dstSet = pongDescriptorSet,
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = static_cast<uint32_t>(pongDescriptorInfos.size()),
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.pImageInfo = nullptr,
				.pBufferInfo = &*pongDescriptorInfos.begin(),
				.pTexelBufferView = nullptr
			}, vk::WriteDescriptorSet{
				.dstSet = pongDescriptorSet,
				.dstBinding = 2,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eUniformBuffer,
				.pImageInfo = nullptr,
				.pBufferInfo = &gridDataBufferInfo,
				.pTexelBufferView = nullptr
			}}, {});
		}
	}
	{
		auto& gridDataInitBuffer = initContext.createInitData<Buffer<GridData>>(
			_context,
			1,
			vk::BufferUsageFlagBits::eTransferSrc,
			MemoryType::TRANSIENT
		);
		*gridDataInitBuffer.data() = {};
		gridDataInitBuffer.flush();

		auto& initCmdBuf = *initContext.initCommandBuffer;

		initCmdBuf.pipelineBarrier(
			vk::PipelineStageFlagBits::eHost,
			vk::PipelineStageFlagBits::eTransfer,
			{},
			{
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eHostWrite,
					.dstAccessMask = vk::AccessFlagBits::eTransferRead
				}
			},
			{}, {}
		);

		initCmdBuf.copyBuffer(gridDataInitBuffer.buffer(), gridDataBuffer.buffer(), {
			vk::BufferCopy {
				.srcOffset = 0,
				.dstOffset = 0,
				.size = sizeof(GridData)
			}
		});

		initCmdBuf.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eComputeShader,
			{},
			{
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
					.dstAccessMask = vk::AccessFlagBits::eUniformRead
				}
			},
			{}, {}
		);
	}
}

void Simulation::run(vk::CommandBuffer buf)
{
	_radixSort.stage(buf, 30, pingDescriptorSet, pongDescriptorSet);
}

}