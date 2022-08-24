//
// Created by daniel on 04.07.22.
//

#include <cstdint>
#include <numeric>

#include "Renderer.h"
#include "Scene.h"
#include "Quad.h"
#include "IndirectCommandsBuffer.h"

#include "descriptors/DescriptorSetLayout.h"

namespace pbf {

Quad::Quad(InitContext& initContext, pbf::Scene& scene) : scene(scene) {

    buffer = {scene.context(), 4,
              vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, pbf::MemoryType::STATIC};
    indexBuffer = {scene.context(), 6, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                   pbf::MemoryType::STATIC};

    graphicsPipeline = scene.context().cache().fetch(
		pbf::descriptors::GraphicsPipeline{
            .shaderStages = {
                    {
                            .stage = vk::ShaderStageFlagBits::eVertex,
                            .module = scene.context().cache().fetch(
								descriptors::ShaderModule{
                                    .source = descriptors::ShaderModule::File{"shaders/particle.vert.spv"},
                                    PBF_DESC_DEBUG_NAME("shaders/particle.vert.spv Vertex Shader")
	                            }),
                            .entryPoint = "main"
                    },
                    {
                            .stage = vk::ShaderStageFlagBits::eFragment,
                            .module = scene.context().cache().fetch(
								descriptors::ShaderModule{
									.source = descriptors::ShaderModule::File{"shaders/particle.frag.spv"},
                                    PBF_DESC_DEBUG_NAME("shaders/particle.frag.spv Fragment Shader")
	                            }),
                            .entryPoint = "main"
                    }
            },
            .vertexBindingDescriptions = {
                    vk::VertexInputBindingDescription{
						.binding = 0,
						.stride = sizeof(VertexData),
						.inputRate = vk::VertexInputRate::eVertex
					},
					vk::VertexInputBindingDescription{
						.binding = 1,
						.stride = sizeof(Scene::ParticleData),
						.inputRate = vk::VertexInputRate::eInstance
					}
			},
            .vertexInputAttributeDescriptions = {
                    vk::VertexInputAttributeDescription{
						.location = 0,
						.binding = 0,
						.format = vk::Format::eR32G32B32Sfloat,
						.offset = 0
					},
					vk::VertexInputAttributeDescription{
						.location = 1,
						.binding = 1,
						.format = vk::Format::eR32G32B32Sfloat,
						.offset = offsetof(Scene::ParticleData, position)
					},
					vk::VertexInputAttributeDescription{
						.location = 2,
						.binding = 1,
						.format = vk::Format::eR32Sfloat,
						.offset = offsetof(Scene::ParticleData, aux)
					}
			},
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
            .pipelineLayout = scene.context().cache().fetch(
				pbf::descriptors::PipelineLayout{
                    .setLayouts = {{
                                           scene.context().globalDescriptorSetLayout()
                                   }},
                    PBF_DESC_DEBUG_NAME("Dummy Pipeline Layout")
            }),
            .renderPass = scene.context().renderer().renderPass(),
            PBF_DESC_DEBUG_NAME("Main Renderer Graphics Pipeline")
    });




	auto& initBuffer = initContext.createInitData<pbf::Buffer<std::byte>>(
		scene.context(), sizeof(VertexData) * 4 + sizeof(std::uint16_t) * 6, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
	);

	VertexData *data = reinterpret_cast<VertexData *>(initBuffer.data());
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
	std::uint16_t *indices = reinterpret_cast<std::uint16_t*>(reinterpret_cast<intptr_t>(initBuffer.data()) + sizeof(VertexData) * 4);
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 0;
	indices[4] = 2;
	indices[5] = 3;
	initBuffer.flush();
	auto& initCmdBuf = initContext.initCommandBuffer;
	initCmdBuf->copyBuffer(initBuffer.buffer(), buffer.buffer(), {
		vk::BufferCopy {
			0, 0, buffer.deviceSize()
		}
	});
	initCmdBuf->copyBuffer(initBuffer.buffer(), indexBuffer.buffer(), {
		vk::BufferCopy {
			buffer.deviceSize(), 0, indexBuffer.deviceSize()
		}
	});
	initCmdBuf->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexInput, {}, {}, {vk::BufferMemoryBarrier{
		.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.buffer = buffer.buffer(),
		.offset = 0,
		.size = buffer.deviceSize()
	}, vk::BufferMemoryBarrier{
		.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask = vk::AccessFlagBits::eIndexRead,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.buffer = indexBuffer.buffer(),
		.offset = 0,
		.size = indexBuffer.deviceSize()
	}}, {});

}

void Quad::frame(uint32_t instanceCount) {
	if (active) {
		indirectCommandsBuffer = scene.getIndirectCommandBuffer(
			graphicsPipeline,
			{},
			std::make_tuple(pbf::BufferRef<std::uint16_t>{&indexBuffer}, vk::IndexType::eUint16),
			{
				BufferRef<VertexData>{&buffer},
				BufferRef<VertexData>{
					.buffer = scene.particleData().as<VertexData>(),
					.offset = sizeof(Scene::ParticleData) * scene.getNumParticles() * scene.context().renderer().currentFrameSync()
				}
			}
		);
        indirectCommandsBuffer->push_back({6, instanceCount, 0, 0});
    }
}

}
