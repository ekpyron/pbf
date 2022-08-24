#include "Simulation.h"
#include "Context.h"
#include "Renderer.h"
#include "Scene.h"

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

Simulation::Simulation(InitContext &initContext, RingBuffer<ParticleData>& particleData):
_context(initContext.context),
_particleData(particleData),
_radixSort(_context, blockSize, getNumParticles() / blockSize, radixSortDescriptorSetLayout(), "shaders/particlesort"),
_gridDataBuffer(_context, 1, vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst, MemoryType::STATIC),
_tempBuffer(_context, particleData.size(), 2, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc, MemoryType::STATIC)
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
		std::vector pingPongLayouts(2 + particleData.segments(), *_context.cache().fetch(radixSortDescriptorSetLayout()));
		initDescriptorSets = _context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = *_descriptorPool,
			.descriptorSetCount = size32(pingPongLayouts),
			.pSetLayouts = pingPongLayouts.data()
		});
		pingDescriptorSet = initDescriptorSets[initDescriptorSets.size() - 2];
		pongDescriptorSet = initDescriptorSets[initDescriptorSets.size() - 1];
		initDescriptorSets.resize(initDescriptorSets.size() - 2);
	}

	vk::DescriptorBufferInfo gridDataBufferInfo{
		.buffer = _gridDataBuffer.buffer(),
		.offset = 0u,
		.range = sizeof(GridData)
	};
	for (size_t i = 0; i < particleData.segments(); ++i)
	{
		auto initDescriptorSet = initDescriptorSets[i];
		auto initDescriptorInfos = {
			particleData.segment(i),
			_tempBuffer.segment(1)
		};
		_context.device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = initDescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(initDescriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*initDescriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}, vk::WriteDescriptorSet{
			.dstSet = initDescriptorSet,
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &gridDataBufferInfo,
			.pTexelBufferView = nullptr
		}}, {});
	}

	{
		auto pingDescriptorInfos = {
			_tempBuffer.segment(0),
			_tempBuffer.segment(1)
		};
		auto pongDescriptorInfos = {
			_tempBuffer.segment(1),
			_tempBuffer.segment(0)
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

		initCmdBuf.copyBuffer(gridDataInitBuffer.buffer(), _gridDataBuffer.buffer(), {
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
	auto sortResult = _radixSort.stage(buf, 30, initDescriptorSets[_context.renderer().currentFrameSync()], pingDescriptorSet, pongDescriptorSet);

	buf.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader,
		vk::PipelineStageFlagBits::eTransfer,
		{},
		{
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eTransferRead
			}
		},
		{},
		{}
	);
	buf.copyBuffer(_tempBuffer.buffer(), _particleData.buffer(), {
		vk::BufferCopy {
			.srcOffset = _tempBuffer.segment(sortResult == RadixSort::Result::InPingBuffer ? 0 : 1).offset,
			.dstOffset = _particleData.segment(_context.renderer().currentFrameSync() + 1).offset,
			.size = _particleData.segmentDeviceSize()
		}
	});
	buf.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eVertexInput,
		{},
		{
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
				.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead
			}
		},
		{},
		{}
	);
}

}