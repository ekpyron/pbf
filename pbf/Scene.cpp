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

Scene::Scene(InitContext &initContext)
: _context(initContext.context),
_particleData{
	_context,
	_numParticles,
	_context.renderer().framePrerenderCount(),
	// TODO: maybe remove transfer src
	vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer,
	pbf::MemoryType::STATIC
},
quad(initContext, *this),
_simulation(initContext, _particleData)
{
	auto& initBuffer = initContext.createInitData<Buffer<ParticleData>>(
		_context, _numParticles, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
	);

	ParticleData* data = initBuffer.data();

	for (int32_t x = 0; x < 64; ++x)
		for (int32_t y = 0; y < 64; ++y)
			for (int32_t z = 0; z < 64; ++z)
			{
				auto id = (64 * 64 * y + 64 * z + x);
				data[id].position = 0.33f * glm::vec3(x - 32, y - 32, z - 32);
				//data[id].position = glm::vec3(x - 32, y - 32, z - 32);
			}
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::shuffle(data, data + 64*64*64, gen);
	initBuffer.flush();

	auto& initCmdBuf = *initContext.initCommandBuffer;
	initCmdBuf.copyBuffer(initBuffer.buffer(), _particleData.buffer(), {
		vk::BufferCopy {
			.srcOffset = 0,
			.dstOffset = sizeof(ParticleData) * _numParticles * (_context.renderer().framePrerenderCount() - 1),
			.size = initBuffer.deviceSize()
		}
	});

	initCmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {vk::BufferMemoryBarrier{
		.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask = vk::AccessFlagBits::eShaderRead,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.buffer = _particleData.buffer(),
		.offset = sizeof(ParticleData) * _numParticles * (_context.renderer().framePrerenderCount() - 1),
		.size = initBuffer.deviceSize(),
	}}, {});

}


void Scene::frame(vk::CommandBuffer &buf) {
    for(auto* ptr: indirectCommandBuffers) ptr->clear();

    quad.frame(_numParticles);
}

void Scene::enqueueCommands(vk::CommandBuffer &buf) {
    const auto &device = _context.device();

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
