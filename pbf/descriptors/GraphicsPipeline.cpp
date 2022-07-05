/**
 *
 *
 * @file GraphicsPipeline.cpp
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#include "GraphicsPipeline.h"

#include <pbf/Context.h>
#include <ranges>

#include <crampl/RangeConversion.h>

using namespace pbf::descriptors;

vk::UniquePipeline GraphicsPipeline::realize(Context* context) const {
    auto const& device = context->device();

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
    vk::PipelineInputAssemblyStateCreateInfo assemblyStateCreateInfo {
		.topology = primitiveTopology,
		.primitiveRestartEnable = primitiveRestartEnable
    };
    vk::PipelineTessellationStateCreateInfo tesselationStateCreateInfo {
		.patchControlPoints = tessellationPatchControlPoints
    };

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo {
		.viewportCount = 1u,
		.pViewports = &viewport,
		.scissorCount = 1u,
		.pScissors = &scissor
	};

	auto stageCreateInfos = shaderStages | std::ranges::views::transform(&ShaderStage::createInfo) | crampl::to<std::vector<vk::PipelineShaderStageCreateInfo>>;
    {
        vertexInputStateCreateInfo
                .setPVertexBindingDescriptions(vertexBindingDescriptions.data())
                .setVertexBindingDescriptionCount(static_cast<uint32_t>(vertexBindingDescriptions.size()))
                .setPVertexAttributeDescriptions(vertexInputAttributeDescriptions.data())
                .setVertexAttributeDescriptionCount(static_cast<uint32_t>(vertexInputAttributeDescriptions.size()));
    }
    {
        colorBlendStateCreateInfo
                .setAttachmentCount(static_cast<uint32_t>(colorBlendAttachmentStates.size()))
                .setPAttachments(colorBlendAttachmentStates.data());
    }

    std::optional<vk::PipelineDynamicStateCreateInfo> dynamicStateCreateInfo;
    if (!dynamicStates.empty()) {
        dynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo {
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates = dynamicStates.data()
        };
    }

    colorBlendStateCreateInfo.setLogicOpEnable(blendLogicOp ? true : false);
    if (blendLogicOp) {
        colorBlendStateCreateInfo.setLogicOp(*blendLogicOp);
    }
    colorBlendStateCreateInfo.setBlendConstants(blendConstants);


    vk::GraphicsPipelineCreateInfo info{
		.stageCount = static_cast<uint32_t>(stageCreateInfos.size()),
		.pStages = stageCreateInfos.data(),
		.pVertexInputState = &vertexInputStateCreateInfo,
		.pInputAssemblyState = &assemblyStateCreateInfo,
		.pTessellationState = &tesselationStateCreateInfo,
		.pViewportState = &viewportStateCreateInfo,
		.pRasterizationState = &rasterizationStateCreateInfo,
		.pMultisampleState = &multisampleStateCreateInfo,
		.pDepthStencilState = &depthStencilStateCreateInfo,
		.pColorBlendState = &colorBlendStateCreateInfo,
		.pDynamicState = dynamicStateCreateInfo ? &*dynamicStateCreateInfo : nullptr,
		.layout = *pipelineLayout,
		.renderPass = *renderPass,
		.subpass = subpass
	};

	auto [result, pipeline] = device.createGraphicsPipelineUnique(nullptr, info);
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("could not create graphics pipeline.");
	return std::move(pipeline);
}
