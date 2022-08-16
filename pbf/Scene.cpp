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

namespace pbf {

Scene::Scene(Context *context)
: _context(context), _particleData{
	context,
	_numParticles * context->renderer().framePrerenderCount(),
	// TODO: maybe remove transfer src
	vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer,
	pbf::MemoryType::STATIC
} {

	for (size_t i = 0u; i < context->renderer().framePrerenderCount(); ++i) {
		_simulations.emplace_back(
			context,
			_numParticles,
			vk::DescriptorBufferInfo{
				.buffer = _particleData.buffer(),
				.offset = ((i + context->renderer().framePrerenderCount() - 1) % context->renderer().framePrerenderCount()) * sizeof(ParticleData) * _numParticles,
				.range = sizeof(ParticleData) * _numParticles
			},
			vk::DescriptorBufferInfo{
				.buffer = _particleData.buffer(),
				.offset = i * sizeof(ParticleData) * _numParticles,
				.range = sizeof(ParticleData) * _numParticles
			}
		);
	}

	quad = std::make_unique<Quad>(this);
}


void Scene::frame(vk::CommandBuffer &buf) {

	if (!_initialized) {

		Buffer<ParticleData> initializeBuffer {_context, _numParticles, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT};

		ParticleData* data = initializeBuffer.data();

		for (int32_t x = 0; x < 64; ++x)
			for (int32_t y = 0; y < 64; ++y)
				for (int32_t z = 0; z < 64; ++z)
				{
					auto id = (64 * 64 * z + 64 * y + x);
					data[id].position = glm::vec3(x - 32, y - 32, z - 32);
				}
		initializeBuffer.flush();

		buf.copyBuffer(initializeBuffer.buffer(), _particleData.buffer(), {
			vk::BufferCopy {
				.srcOffset = 0,
				.dstOffset = sizeof(ParticleData) * _numParticles * (_context->renderer().framePrerenderCount() - 1),
				.size = initializeBuffer.deviceSize()
			}
		});

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {vk::BufferMemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.buffer = _particleData.buffer(),
			.offset = sizeof(ParticleData) * _numParticles * (_context->renderer().framePrerenderCount() - 1),
			.size = initializeBuffer.deviceSize(),
		}}, {});


		// keep alive until next use of frame sync
		_context->renderer().stage([initializeBuffer = std::move(initializeBuffer)] (auto) {});


		_initialized = true;
	}

    for(auto* ptr: indirectCommandBuffers) ptr->clear();

    quad->frame(_numParticles);
}

void Scene::enqueueCommands(vk::CommandBuffer &buf) {
    const auto &device = _context->device();

    for (auto& [graphicsPipeline, innerMap] : indirectDrawCalls)
    {
        buf.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
        buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *(graphicsPipeline.descriptor().pipelineLayout), 0, { _context->globalDescriptorSet()}, {});

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
