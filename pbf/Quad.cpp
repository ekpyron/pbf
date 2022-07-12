//
// Created by daniel on 04.07.22.
//

#include "descriptors/DescriptorSetLayout.h"
#include "Renderer.h"
#include "Scene.h"
#include <cstdint>
#include <numeric>
#include "Quad.h"
#include "IndirectCommandsBuffer.h"

namespace pbf {

Quad::Quad(pbf::Scene *scene) : scene(scene) {

    buffer = {scene->context(), sizeof(VertexData) * 4,
              vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, pbf::MemoryType::STATIC};
    indexBuffer = {scene->context(), sizeof(std::uint16_t) * 6, vk::BufferUsageFlagBits::eTransferDst
                                                                | vk::BufferUsageFlagBits::eIndexBuffer,
                   pbf::MemoryType::STATIC};

    graphicsPipeline = scene->context()->cache().fetch(
		pbf::descriptors::GraphicsPipeline{
            .shaderStages = {
                    {
                            .stage = vk::ShaderStageFlagBits::eVertex,
                            .module = scene->context()->cache().fetch(
								pbf::descriptors::ShaderModule{
                                    .filename = "shaders/triangle.vert.spv",
                                    PBF_DESC_DEBUG_NAME("shaders/triangle.vert.spv Vertex Shader")
                            }),
                            .entryPoint = "main"
                    },
                    {
                            .stage = vk::ShaderStageFlagBits::eFragment,
                            .module = scene->context()->cache().fetch(
								pbf::descriptors::ShaderModule{
                                    .filename = "shaders/triangle.frag.spv",
                                    PBF_DESC_DEBUG_NAME("Fragment Shader shaders/triangle.frag.spv")
                            }),
                            .entryPoint = "main"
                    }
            },
            .vertexBindingDescriptions = {
                    vk::VertexInputBindingDescription{0, sizeof(VertexData), vk::VertexInputRate::eVertex}},
            .vertexInputAttributeDescriptions = {
                    vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, 0}},
            .primitiveTopology = vk::PrimitiveTopology::eTriangleList,
            .rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo().setLineWidth(1.0f),
			.depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo{
				.depthTestEnable = true,
				.depthWriteEnable = true,
				.depthCompareOp = vk::CompareOp::eLess,
				.depthBoundsTestEnable = false,
				.stencilTestEnable = false,
				.minDepthBounds = 0.0f,
				.maxDepthBounds = 1.0f
			},
			.colorBlendAttachmentStates = {
                    vk::PipelineColorBlendAttachmentState().setColorWriteMask(
                            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
                    )
            },
            .dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
            .pipelineLayout = scene->context()->cache().fetch(
				pbf::descriptors::PipelineLayout{
                    .setLayouts = {{
                                           scene->context()->globalDescriptorSetLayout()
                                   }},
				   .pushConstants = {vk::PushConstantRange{
					   .stageFlags = vk::ShaderStageFlagBits::eVertex,
					   .offset = 0,
					   .size = sizeof(PushConstantData)
				   }},
                    PBF_DESC_DEBUG_NAME("Dummy Pipeline Layout")
            }),
            .renderPass = scene->context()->renderer().renderPass(),
            PBF_DESC_DEBUG_NAME("Main Renderer Graphics Pipeline")
    });

    // enqueue functor

    /*TypedClosureContainer container([
                                       this, initializeBufferT = std::move(initializeBuffer)
                               ] (vk::CommandBuffer& enqueueBuffer) {
        enqueueBuffer.copyBuffer(initializeBufferT.buffer(), buffer.buffer(), {
                vk::BufferCopy {
                        0, 0, buffer.size()
                }
        });
        enqueueBuffer.copyBuffer(initializeBufferT.buffer(), indexBuffer.buffer(), {
                vk::BufferCopy {
                        sizeof(VertexData) * 3, 0, indexBuffer.size()
                }
        });
    });*/

}

void Quad::frame(uint32_t instanceCount) {
	if (active) {
		indirectCommandsBuffer = scene->getIndirectCommandBuffer(
			graphicsPipeline,
			PushConstantData{
				.startIndex = scene->context()->renderer().currentFrameSync() * scene->getNumParticles()
			},
			std::make_tuple(pbf::BufferRef{&indexBuffer}, vk::IndexType::eUint16),
			{pbf::BufferRef{&buffer}}
		);
        indirectCommandsBuffer->push_back({6, instanceCount, 0, 0});
    }


	if (dirty) {
        pbf::Buffer initializeBuffer {scene->context(), sizeof(VertexData) * 4 + sizeof(std::uint16_t) * 6, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT};

        VertexData *data = reinterpret_cast<VertexData *>(initializeBuffer.data());
        data[0].vertices[0]= -1.0f;
        data[0].vertices[1]= -1.0f;
        data[0].vertices[2]= 0.0f;
		data[1].vertices[0]= 1.0f;
		data[1].vertices[1]= -1.0f;
		data[1].vertices[2] = 0.0f;
        data[2].vertices[0]= 1.0f;
        data[2].vertices[1]= 1.0f;
        data[2].vertices[2]= 0.0f;
        data[3].vertices[0]= -1.0f;
        data[3].vertices[1]= 1.0f;
        data[3].vertices[2] = 0.0f;
        std::uint16_t *indices = reinterpret_cast<std::uint16_t*>(reinterpret_cast<intptr_t>(initializeBuffer.data()) + sizeof(VertexData) * 4);
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
		indices[3] = 0;
		indices[4] = 2;
		indices[5] = 3;
        initializeBuffer.flush();
        scene->context()->renderer().stage([
                                                   this, initializeBuffer = std::move(initializeBuffer)
                                           ] (vk::CommandBuffer& enqueueBuffer) {
            enqueueBuffer.copyBuffer(initializeBuffer.buffer(), buffer.buffer(), {
                    vk::BufferCopy {
                            0, 0, buffer.size()
                    }
            });
            enqueueBuffer.copyBuffer(initializeBuffer.buffer(), indexBuffer.buffer(), {
                    vk::BufferCopy {
                            sizeof(VertexData) * 4, 0, indexBuffer.size()
                    }
            });

			enqueueBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexInput, {}, {}, {vk::BufferMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
				.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead,
				.srcQueueFamilyIndex = 0,
				.dstQueueFamilyIndex = 0,
				.buffer = buffer.buffer(),
				.offset = 0,
				.size = buffer.size()
			}, vk::BufferMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
				.dstAccessMask = vk::AccessFlagBits::eIndexRead,
				.srcQueueFamilyIndex = 0,
				.dstQueueFamilyIndex = 0,
				.buffer = indexBuffer.buffer(),
				.offset = 0,
				.size = indexBuffer.size()
			}}, {});
        });

        dirty = false;
    }

}

}
