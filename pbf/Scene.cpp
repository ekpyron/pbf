/**
 *
 *
 * @file Scene.cpp
 * @brief 
 * @author clonker
 * @date 1/23/19
 */
#include <numeric>
#include <cstdint>

#include "Scene.h"
#include "Renderer.h"
#include "Simulation.h"

#include "descriptors/GraphicsPipeline.h"
#include "descriptors/DescriptorSetLayout.h"
#include "IndirectCommandsBuffer.h"
#include <random>

namespace pbf {

void initializeParticleData(ParticleData* data, size_t numParticles)
{
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<float> dist(-0.25f, 0.25f);
	for (int32_t x = 0; x < 32; ++x)
		   for (int32_t y = 0; y < 32; ++y)
				   for (int32_t z = 0; z < 32; ++z)
				   {
						   auto id = (32 * 32 * y + 32 * z + x);
						   //data[id].position = 0.33f * glm::vec3(x - 32, y - 32, z - 32);
						   data[id].position = glm::vec3(x - 32, y - 32, z - 32);
						   data[id].position += glm::vec3(dist(gen), dist(gen), dist(gen));
				   }
	std::shuffle(data, data + numParticles, gen);
}

Scene::Scene(InitContext &initContext)
: _context(initContext.context),
_particleData([&](){
	auto& context = initContext.context;
	RingBuffer<ParticleData> particleData{
		context,
		_numParticles,
		context.renderer().framePrerenderCount(),
		// TODO: maybe remove transfer src
		vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer,
		pbf::MemoryType::STATIC
	};

	auto& initBuffer = initContext.createInitData<Buffer<ParticleData>>(
		context, _numParticles, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
	);

	ParticleData* data = initBuffer.data();
	initializeParticleData(data, _numParticles);
	initBuffer.flush();

	auto& initCmdBuf = *initContext.initCommandBuffer;
	for (size_t i = 0; i < context.renderer().framePrerenderCount(); ++i)
		initCmdBuf.copyBuffer(initBuffer.buffer(), particleData.buffer(), {
			vk::BufferCopy {
				.srcOffset = 0,
				.dstOffset = sizeof(ParticleData) * _numParticles * i,
				.size = initBuffer.deviceSize()
			}
		});

	initCmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {vk::BufferMemoryBarrier{
		.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask = vk::AccessFlagBits::eShaderRead,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.buffer = particleData.buffer(),
		.offset = 0,
		.size = particleData.deviceSize(),
	}}, {});
	return particleData;
}()),
quad(initContext, *this),
_simulation(initContext, _particleData)
{
}

void Scene::resetParticles()
{
	_resetParticles = true;
	_simulation.resetKeys();
}

void Scene::frame(vk::CommandBuffer &buf) {
    for(auto* ptr: indirectCommandBuffers) ptr->clear();

    quad.frame(_numParticles);
}

void Scene::enqueueCommands(vk::CommandBuffer &buf) {
    const auto &device = _context.device();

	if (_resetParticles)
	{
		auto& initBuffer = _context.renderer().createFrameData<Buffer<ParticleData>>(
			_context, _numParticles, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
		);
		ParticleData* data = initBuffer.data();

		initializeParticleData(data, _numParticles);
		initBuffer.flush();

		for (size_t i = 0; i < _context.renderer().framePrerenderCount(); ++i)
			buf.copyBuffer(initBuffer.buffer(), _particleData.buffer(), {
				vk::BufferCopy {
					.srcOffset = 0,
					.dstOffset = sizeof(ParticleData) * _numParticles * i,
					.size = initBuffer.deviceSize()
				}
			});

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {vk::BufferMemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.buffer = _particleData.buffer(),
			.offset = 0,
			.size = _particleData.deviceSize(),
		}}, {});
		_resetParticles = false;
	}


    for (auto& [graphicsPipeline, innerMap] : indirectDrawCalls)
    {
        buf.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
        buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *(graphicsPipeline.descriptor().pipelineLayout), 0, { _context.globalDescriptorSet()}, {});

		for (auto& [pushConstantData, innerMap] : innerMap)
		{
			if (!pushConstantData.empty())
				buf.pushConstants(
					*(graphicsPipeline.descriptor().pipelineLayout),
					vk::ShaderStageFlagBits::eVertex,
					0,
					static_cast<uint32_t>(pushConstantData.size()),
					pushConstantData.data()
				);
			for (auto &[index, innerMap]: innerMap)
			{
				auto &[indexBuffer, indexType] = index;
				buf.bindIndexBuffer(indexBuffer.buffer->buffer(), 0, indexType);
				for (auto &[vertexBufferRefs, indirectCommandBuffer]: innerMap)
				{
					std::vector<vk::Buffer> vkBuffers;
					std::vector<vk::DeviceSize> offsets;
					for (BufferRef<Quad::VertexData> const& vertexBufferRef: vertexBufferRefs) {
						vkBuffers.emplace_back(vertexBufferRef.buffer->buffer());
						offsets.emplace_back(vertexBufferRef.offset);
					}
					buf.bindVertexBuffers(0, vkBuffers, offsets);

					{
						auto it = indirectCommandBuffer.buffers().begin();
						while (it != indirectCommandBuffer.buffers().end())
						{
							auto next = it;
							++next;

							auto lastBuffer = next == indirectCommandBuffer.buffers().end();
							buf.drawIndexedIndirect(
								it->buffer(), 0,
								lastBuffer ?
									indirectCommandBuffer.elementsInLastBuffer():
									IndirectCommandsBuffer::bufferSize, sizeof(vk::DrawIndirectCommand));
							it = next;
						}

					}
				}
			}
		}

    }

}


}
