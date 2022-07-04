/**
 *
 *
 * @file GraphicsPipeline.h
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#pragma once

#include <pbf/common.h>
#include <pbf/Cache.h>
#include <pbf/descriptors/ShaderStage.h>
#include <pbf/descriptors/ShaderModule.h>
#include <pbf/descriptors/PipelineLayout.h>
#include <pbf/descriptors/RenderPass.h>

namespace pbf::descriptors {

struct GraphicsPipeline {
    vk::UniquePipeline realize(Context* context) const;

    template<typename T = GraphicsPipeline>
    using Compare = PBFMemberComparator<&T::shaderStages, &T::vertexBindingDescriptions, &T::vertexInputAttributeDescriptions,
                &T::primitiveTopology, &T::primitiveRestartEnable, &T::tessellationPatchControlPoints,
                &T::rasterizationStateCreateInfo, &T::multisampleStateCreateInfo, &T::depthStencilStateCreateInfo,
                &T::colorBlendAttachmentStates,
                &T::dynamicStates, &T::blendConstants, &T::blendLogicOp, &T::subpass,
                &T::viewport, &T::scissor, &T::pipelineLayout, &T::renderPass>;

    std::vector<ShaderStage> shaderStages;
    std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions;
    std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions;
    vk::PrimitiveTopology primitiveTopology = vk::PrimitiveTopology::eTriangleList;
    bool primitiveRestartEnable = false;
    std::uint32_t tessellationPatchControlPoints = 0;
    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo().setLineWidth(1.0f);
    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachmentStates;
    std::vector<vk::DynamicState> dynamicStates;
    std::array<float, 4> blendConstants = {{ 0, 0, 0, 0 }};
    std::optional<vk::LogicOp> blendLogicOp = {};

    std::uint32_t subpass { 0 };
    vk::Viewport viewport = {};
    vk::Rect2D scissor = { {}, { static_cast<std::uint32_t>(viewport.width), static_cast<std::uint32_t>(viewport.height) } };

    CacheReference<PipelineLayout> pipelineLayout;
    CacheReference<RenderPass> renderPass;

#ifndef NDEBUG
    std::string debugName;
#endif

    template<typename T = GraphicsPipeline>
    using Depends = crampl::NonTypeList<&T::renderPass>;
};

}
