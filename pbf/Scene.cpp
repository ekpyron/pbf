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

#include "Scene.h"
#include "Renderer.h"
#include "Simulation.h"

#include "descriptors/DescriptorSetLayout.h"
#include "IndirectCommandsBuffer.h"

namespace pbf {


Scene::Scene(InitContext &initContext)
: _context(initContext.context),
_particleData(initContext.context, _numParticles, initContext.context.renderer().framePrerenderCount(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, pbf::MemoryType::STATIC),
quad(initContext, *this),
_simulation(initContext, _numParticles)
{
}

void Scene::resetParticles()
{
	_resetParticles = true;
	_simulation.resetKeys();
}

void Scene::frame(vk::CommandBuffer &buf) {
	if (_resetParticles)
	{
        simulation().reset(buf);
		_resetParticles = false;
	}

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
