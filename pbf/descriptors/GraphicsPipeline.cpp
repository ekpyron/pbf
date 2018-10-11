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
    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
    vk::PipelineInputAssemblyStateCreateInfo assemblyStateCreateInfo {
            {}, primitiveTopology, primitiveRestartEnable
    };
    vk::PipelineTessellationStateCreateInfo tesselationStateCreateInfo {
            {}, tessellationPatchControlPoints
    };

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo {{}, 1u, &viewport, 1u, &scissor };

    {
        stageCreateInfos.reserve(shaderStages.size());
        for (const auto &stage : shaderStages) {
            stageCreateInfos.emplace_back(vk::PipelineShaderStageCreateInfo{
                    {}, // flags,
                    stage.stage, // stage
                    *stage.module, // shader module
                    stage.entryPoint.c_str(), // entry point
                    nullptr  // specialization info
            });
        }
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
                {}, static_cast<uint32_t>(dynamicStates.size()), dynamicStates.data()
        };
    }

    colorBlendStateCreateInfo.setLogicOpEnable(blendLogicOp ? true : false);
    if (blendLogicOp) {
        colorBlendStateCreateInfo.setLogicOp(*blendLogicOp);
    }
    colorBlendStateCreateInfo.setBlendConstants(blendConstants);


    vk::GraphicsPipelineCreateInfo info({},
                                        static_cast<uint32_t>(stageCreateInfos.size()), stageCreateInfos.data(),
                                        &vertexInputStateCreateInfo, &assemblyStateCreateInfo, &tesselationStateCreateInfo,
                                        &viewportStateCreateInfo, &rasterizationStateCreateInfo, &multisampleStateCreateInfo,
                                        &depthStencilStateCreateInfo, &colorBlendStateCreateInfo,
                                        dynamicStateCreateInfo ? &*dynamicStateCreateInfo : nullptr, *pipelineLayout,
                                        *renderPass, subpass);

    return device.createGraphicsPipelineUnique(nullptr, info);
}
