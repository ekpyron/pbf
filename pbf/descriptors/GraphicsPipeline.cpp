/**
 *
 *
 * @file GraphicsPipeline.cpp
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#include "GraphicsPipeline.h"

#include <pbf/VulkanContext.h>
#include <ranges>

#include <crampl/RangeConversion.h>

using namespace pbf::descriptors;

pbf::Pipeline GraphicsPipeline::realize(ContextInterface &context) const {
	pbf::Pipeline graphicsPipeline;
    auto const& device = context.device();

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

	std::vector<vk::PipelineShaderStageCreateInfo> stageCreateInfos;
	std::vector<vk::SpecializationInfo> specializationInfos;
	stageCreateInfos.reserve(shaderStages.size());
	specializationInfos.reserve(shaderStages.size());
	for (auto const& shaderStage: shaderStages) {
		stageCreateInfos.emplace_back(shaderStage.createInfo(specializationInfos.emplace_back()));
	}
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

	graphicsPipeline.pipelineLayout = Pipeline::deducePipelineLayout(context.cache(),
		shaderStages
		PBF_ARG_DEBUG_NAME(debugName + " Pipeline Layout")
	);

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
		.layout = *graphicsPipeline.pipelineLayout,
		.renderPass = *renderPass,
		.subpass = subpass
	};

	auto [result, pipeline] = device.createGraphicsPipelineUnique(nullptr, info);
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("could not create graphics pipeline.");
	graphicsPipeline.pipeline = std::move(pipeline);
	return graphicsPipeline;
}
