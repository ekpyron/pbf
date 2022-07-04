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

Scene::Scene(Context *context) : _context(context), _simulation(context) {
    quad = std::make_unique<Quad>(this);
}


void Scene::frame() {
    for(auto* ptr: indirectCommandBuffers) ptr->clear();

    quad->frame(_simulation.getNumParticles());
}

void Scene::enqueueCommands(vk::CommandBuffer &buf) {
    const auto &device = _context->device();

    for (auto& [graphicsPipeline, innerMap] : indirectDrawCalls)
    {
        buf.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
        buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *(graphicsPipeline.descriptor().pipelineLayout), 0, { _context->globalDescriptorSet()}, {});

        for (auto& [index, innerMap] : innerMap)
        {
            auto& [indexBuffer, indexType] = index;
            buf.bindIndexBuffer(indexBuffer.buffer->buffer(), 0, indexType);
            for (auto& [vertexBuffers, indirectCommandBuffer] : innerMap)
            {
                std::vector<vk::Buffer> vkBuffers;
                vkBuffers.reserve(vertexBuffers.size());
                std::transform(std::begin(vertexBuffers), std::end(vertexBuffers),
                        std::back_inserter(vkBuffers), [](const auto& vb) {return vb.buffer->buffer();});
                std::vector<vk::DeviceSize> offsets (vertexBuffers.size(), vk::DeviceSize{0});
                assert(offsets.size() == vertexBuffers.size());
                buf.bindVertexBuffers(0, vkBuffers, offsets);

                {
                    auto it = indirectCommandBuffer.buffers().begin();
                    while (it != indirectCommandBuffer.buffers().end())
                    {
                        auto next = it;
                        ++next;

                        auto lastBuffer = next == indirectCommandBuffer.buffers().end();
                        buf.drawIndexedIndirect(it->buffer(), 0,
                                                lastBuffer ? indirectCommandBuffer.elementsInLastBuffer() : IndirectCommandsBuffer::bufferSize
                                , sizeof(vk::DrawIndirectCommand));
                        it = next;
                    }

                }
            }
        }

    }

}


}
