/**
 *
 *
 * @file Scene.cpp
 * @brief 
 * @author clonker
 * @date 1/23/19
 */
#include <cstdint>
#include <random>
#include "App.h"
#include "Scene.h"
#include "Renderer.h"
#include "Simulation.h"

#include "descriptors/DescriptorSetLayout.h"
#include "IndirectCommandsBuffer.h"

namespace pbf {


Scene::Scene(InitContext &initContext, GUI& gui, Renderer& renderer, GlobalAppData& globalData, size_t _numParticles)
: _context(initContext.context), globalData(globalData),
quad(initContext, *this, renderer, globalData)
{
	_particleData = RingBuffer<ParticleData>(initContext.context, _numParticles, renderer.framePrerenderCount(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, pbf::MemoryType::STATIC);
}

void Scene::selectParticle(vk::CommandBuffer buf, size_t _index) {
	for (size_t i = 0; i <= _particleData.segments(); ++i)
	{
		auto segment = _particleData.segment(i);
		buf.fillBuffer(
			segment.buffer,
			segment.offset + sizeof(ParticleData) * _index + offsetof(ParticleData, aux),
			sizeof(ParticleData::aux),
			-1u
		);
	}

}

void Scene::frame(vk::CommandBuffer &buf) {
	for(auto* ptr: indirectCommandBuffers) ptr->clear();

    quad.frame(_particleData.size());
}

void Scene::enqueueCommands(vk::CommandBuffer &buf) {
    const auto &device = _context.device();

    for (auto& [graphicsPipeline, innerMap] : indirectDrawCalls)
    {
        buf.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline->pipeline);
        buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipeline->pipelineLayout, 0, { globalData.globalDescriptorSet()}, {});

		for (auto& [pushConstantData, innerMap] : innerMap)
		{
			if (!pushConstantData.empty())
				buf.pushConstants(
					*graphicsPipeline->pipelineLayout,
					vk::ShaderStageFlagBits::eAll,
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
