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

using namespace pbf::descriptors;

vk::UniquePipeline GraphicsPipeline::realize(Context* context) const {
    auto const& device = context->device();

    std::vector<vk::PipelineShaderStageCreateInfo> stageCreateInfos;
    vk::PipelineInputAssemblyStateCreateInfo assemblyStateCreateInfo {
        .flags = {},
        .topology = primitiveTopology,
        .primitiveRestartEnable = primitiveRestartEnable
    };
    vk::PipelineTessellationStateCreateInfo tesselationStateCreateInfo {
        .flags = {},
        .patchControlPoints = tessellationPatchControlPoints
    };

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo {
        .flags = {},
        .viewportCount = 1u,
        .pViewports = &viewport,
        .scissorCount = 1u,
        .pScissors = &scissor
    };

    {
        stageCreateInfos.reserve(shaderStages.size());
        for (const auto &stage : shaderStages) {
            stageCreateInfos.emplace_back(vk::PipelineShaderStageCreateInfo{
                    .flags = {},
                    .stage = stage.stage,
                    .module = *stage.module,
                    .pName = stage.entryPoint.c_str(),
                    .pSpecializationInfo = nullptr
            });
        }
    }
    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
            .vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size()),
            .pVertexBindingDescriptions = vertexBindingDescriptions.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size()),
            .pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data()
    };
    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
            .logicOpEnable = blendLogicOp ? true : false,
            .logicOp = blendLogicOp ? *blendLogicOp : vk::LogicOp::eClear,
            .attachmentCount = static_cast<uint32_t>(colorBlendAttachmentStates.size()),
            .pAttachments = colorBlendAttachmentStates.data(),
            .blendConstants = blendConstants
    };

    std::optional<vk::PipelineDynamicStateCreateInfo> dynamicStateCreateInfo;
    if (!dynamicStates.empty()) {
        dynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo {
                .flags = {},
                .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                .pDynamicStates = dynamicStates.data()
        };
    }


    vk::GraphicsPipelineCreateInfo info {
            .flags = {},
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

    return device.createGraphicsPipelineUnique(nullptr, info);
}
