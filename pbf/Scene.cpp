/**
 *
 *
 * @file Scene.cpp
 * @brief 
 * @author clonker
 * @date 1/23/19
 */
#include <numeric>

#include "Scene.h"
#include "Renderer.h"

#include "descriptors/GraphicsPipeline.h"
#include "descriptors/DescriptorSetLayout.h"

namespace pbf {

Scene::Scene(Context *context) : _context(context) {

    triangle = std::make_unique<Triangle>(this);
    triangleInstance = std::make_unique<Instance>(triangle.get());


}

void Scene::IndirectCommandsBuffer::clear() {
    _currentBuffer = _buffers.begin();
    _elementsInLastBuffer = 0;
}

Scene::IndirectCommandsBuffer::IndirectCommandsBuffer(pbf::Context *context) :context(context) {
    _buffers.emplace_back(context, sizeof(vk::DrawIndirectCommand) * bufferSize, vk::BufferUsageFlagBits::eIndirectBuffer, MemoryType::DYNAMIC);
    _currentBuffer = _buffers.begin();
    _elementsInLastBuffer = 0;

}

void Scene::IndirectCommandsBuffer::push_back(const vk::DrawIndirectCommand &cmd) {
    if (_elementsInLastBuffer >= bufferSize) {
        ++_currentBuffer;
        if (_currentBuffer == _buffers.end())
        {
            _buffers.emplace_back(context, sizeof(vk::DrawIndirectCommand) * bufferSize, vk::BufferUsageFlagBits::eIndirectBuffer, MemoryType::DYNAMIC);
            _currentBuffer = std::prev(_buffers.end());
        }
        _elementsInLastBuffer = 0;
    }
    reinterpret_cast<vk::DrawIndirectCommand*>(_currentBuffer->data())[_elementsInLastBuffer++] = cmd;

}

void Scene::frame(vk::CommandBuffer &buf) {
    for(auto* ptr: indirectCommandBuffers) ptr->clear();

    triangle->frame(buf);
}

void Scene::enqueueCommands(vk::CommandBuffer &buf) {
    const auto &device = _context->device();

    for (auto const& [graphicsPipeline, innerMap] : indirectDrawCalls)
    {
        buf.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
        buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *(graphicsPipeline.descriptor().pipelineLayout), 0, { _context->globalDescriptorSet()}, {});

        for (auto const& [index, innerMap] : innerMap)
        {
            const auto & [indexBuffer, indexType] = index;
            buf.bindIndexBuffer(indexBuffer.buffer->buffer(), 0, indexType);
            for (auto const& [vertexBuffers, indirectCommandBuffer] : innerMap)
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

Triangle::Triangle(Scene *scene)  :scene(scene) {

    buffer = {scene->context(), sizeof(VertexData)*3,
              vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, MemoryType::STATIC};
    indexBuffer = {scene->context(), sizeof(std::uint16_t)*3, vk::BufferUsageFlagBits::eTransferDst
                   | vk::BufferUsageFlagBits::eIndexBuffer, MemoryType::STATIC};
    initializeBuffer = {scene->context(), sizeof(VertexData)*3 + sizeof(std::uint16_t) * 4, vk::BufferUsageFlagBits::eTransferSrc, MemoryType::TRANSIENT};

    VertexData *data = reinterpret_cast<VertexData *>(initializeBuffer.data());
    data[0].vertices[0]= 0.0f;
    data[0].vertices[1]= 1.0f;
    data[0].vertices[2]= 0.0f;
    data[1].vertices[0]= -1.0f;
    data[1].vertices[1]= -1.0f;
    data[1].vertices[2]= 0.0f;
    data[2].vertices[0]= 1.0f;
    data[2].vertices[1]= -1.0f;
    data[2].vertices[2] = 0.0f;
    std::uint16_t *indices = reinterpret_cast<std::uint16_t*>(reinterpret_cast<std::intptr_t>(initializeBuffer.data()) + sizeof(VertexData) * 3);
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    initializeBuffer.flush();

    const auto &graphicsPipeline = scene->context()->cache().fetch(descriptors::GraphicsPipeline{
            .shaderStages = {
                    {
                            .stage = vk::ShaderStageFlagBits::eVertex,
                            .module = scene->context()->cache().fetch(descriptors::ShaderModule{
                                    .filename = "shaders/triangle.vert.spv",
                                    PBF_DESC_DEBUG_NAME("shaders/triangle.vert.spv Vertex Shader")
                            }),
                            .entryPoint = "main"
                    },
                    {
                            .stage = vk::ShaderStageFlagBits::eFragment,
                            .module = scene->context()->cache().fetch(descriptors::ShaderModule{
                                    .filename = "shaders/triangle.frag.spv",
                                    PBF_DESC_DEBUG_NAME("Fragment Shader shaders/triangle.frag.spv")
                            }),
                            .entryPoint = "main"
                    }
            },
            .vertexBindingDescriptions = {vk::VertexInputBindingDescription{0, sizeof(VertexData), vk::VertexInputRate::eVertex}},
            .vertexInputAttributeDescriptions = {vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, 0}},
            .primitiveTopology = vk::PrimitiveTopology::eTriangleList,
            .rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo().setLineWidth(1.0f),
            .colorBlendAttachmentStates = {
                    vk::PipelineColorBlendAttachmentState().setColorWriteMask(
                            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
                    )
            },
            .dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
            .pipelineLayout = scene->context()->cache().fetch(descriptors::PipelineLayout{
                .setLayouts = {{
                        scene->context()->globalDescriptorSetLayout()
                }},
                    PBF_DESC_DEBUG_NAME("Dummy Pipeline Layout")
            }),
            .renderPass = scene->context()->renderer().renderPass(),
            PBF_DESC_DEBUG_NAME("Main Renderer Graphics Pipeline")
    });

    indirectCommandsBuffer = scene->getIndirectCommandBuffer(graphicsPipeline, std::make_tuple(BufferRef{&indexBuffer}, vk::IndexType::eUint16),
            {BufferRef{&buffer}});

    initializedEvent = scene->context()->device().createEventUnique(vk::EventCreateInfo{});
}

void Triangle::frame(vk::CommandBuffer &enqueueBuffer) {
    if (!isInitialized) {
        auto const& device = scene->context()->device();

        enqueueBuffer.copyBuffer(initializeBuffer.buffer(), buffer.buffer(), {
                vk::BufferCopy {
                        0, 0, buffer.size()
                }
        });
        enqueueBuffer.copyBuffer(initializeBuffer.buffer(), indexBuffer.buffer(), {
                vk::BufferCopy {
                        sizeof(VertexData) * 3, 0, indexBuffer.size()
                }
        });
        enqueueBuffer.setEvent(*initializedEvent, vk::PipelineStageFlagBits::eTransfer);

        // copy

    }
    if (active && instanceCount() > 0) {
        if (dirty) {
            enqueueBuffer.waitEvents(
                    {*initializedEvent}, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexInput, {}, {vk::BufferMemoryBarrier{
                            vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eVertexAttributeRead, 0, 0, buffer.buffer(), 0, buffer.size()
                    }, vk::BufferMemoryBarrier{
                            vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eIndexRead, 0, 0, indexBuffer.buffer(), 0, indexBuffer.size()
                    }}, {}
            );
            dirty = false;
        }
        // if (potentiallyVisisble() && occlusionQuery()) {
        // draw
        // }
        indirectCommandsBuffer->push_back({3, instanceCount(), 0, 0});
    }
}
}
