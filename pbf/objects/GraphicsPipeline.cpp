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

namespace pbf::objects {


GraphicsPipeline::GraphicsPipeline(Context *context, const GraphicsPipeline::Descriptor &descriptor) {
    const auto &device = context->device();
    _pipeline = descriptor.realize(device);
}

vk::UniquePipeline GraphicsPipeline::Descriptor::realize(const vk::Device &device) const {
    std::vector<vk::PipelineShaderStageCreateInfo> _stageCreateInfos;
    vk::PipelineVertexInputStateCreateInfo _vertexInputStateCreateInfo;
    vk::PipelineColorBlendStateCreateInfo _colorBlendStateCreateInfo;
    vk::PipelineDynamicStateCreateInfo _dynamicStateCreateInfo;

    vk::PipelineViewportStateCreateInfo _viewportStateCreateInfo {{}, 1u, &_viewport, 1u, &_scissor};

    {
        _stageCreateInfos.clear();
        _stageCreateInfos.reserve(_stages.size());
        for (const auto &stage : _stages) {
            _stageCreateInfos.emplace_back(vk::PipelineShaderStageCreateInfo{
                    {}, // flags,
                    std::get<0>(stage), // stage
                    std::get<1>(stage)->get(), // shader module
                    std::get<2>(stage).c_str(), // entry point
                    nullptr  // specialization info
            });
        }
    }
    {
        _vertexInputStateCreateInfo
                .setPVertexBindingDescriptions(_bindingDescriptions.data())
                .setVertexBindingDescriptionCount(static_cast<uint32_t>(_bindingDescriptions.size()))
                .setPVertexAttributeDescriptions(_vertexInputAttributeDescriptions.data())
                .setVertexAttributeDescriptionCount(static_cast<uint32_t>(_vertexInputAttributeDescriptions.size()));
    }
    {
        _colorBlendStateCreateInfo
                .setAttachmentCount(static_cast<uint32_t>(_colorBlendAttachmentStates.size()))
                .setPAttachments(_colorBlendAttachmentStates.data());
    }

    _dynamicStateCreateInfo.setDynamicStateCount(static_cast<uint32_t>(_dynamicStates.size()));
    _dynamicStateCreateInfo.setPDynamicStates(_dynamicStates.empty() ? nullptr : _dynamicStates.data());

    _colorBlendStateCreateInfo.setLogicOpEnable(static_cast<vk::Bool32>(_blendLogicOpEnabled)).setLogicOp(_blendLogicOp);
    _colorBlendStateCreateInfo.setBlendConstants(_blendConstants);


    vk::GraphicsPipelineCreateInfo info({},
                                        static_cast<uint32_t>(_stageCreateInfos.size()), _stageCreateInfos.data(),
                                        &_vertexInputStateCreateInfo, &_assemblyStateCreateInfo, &_tesselationStateCreateInfo,
                                        &_viewportStateCreateInfo, &_rasterizationStateCreateInfo, &_multisampleStateCreateInfo,
                                        &_depthStencilStateCreateInfo, &_colorBlendStateCreateInfo,
                                        _dynamicStates.empty() ? nullptr : &_dynamicStateCreateInfo, _pipelineLayout->get(),
                                        _renderPass->get(), _subpass);

    return device.createGraphicsPipelineUnique(nullptr, info);
}
}

