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
#include <pbf/objects/ShaderModule.h>
#include <pbf/objects/PipelineLayout.h>
#include <pbf/objects/RenderPass.h>

namespace pbf::objects {

class GraphicsPipeline {
public:
    vk::UniquePipeline realize(Context* context) const;

    void
    addShaderStage(vk::ShaderStageFlagBits stage, CacheReference<ShaderModule> shader, std::string_view entryPoint) {
        _stages.emplace_back(stage, shader, entryPoint);
    }

    template<typename... Args>
    void addVertexInputBindingDescriptions(Args&& ... args) {
        _bindingDescriptions.emplace_back(std::forward<Args>(args)...);
    }

    template<typename... Args>
    void addVertexInputAttributeDescriptions(Args&& ... args) {
        _vertexInputAttributeDescriptions.emplace_back(std::forward<Args>(args)...);
    }

    void setTopology(vk::PrimitiveTopology topology) {
        _assemblyStateCreateInfo.setTopology(topology);
    }

    void setPrimitiveRestart(bool value) {
        _assemblyStateCreateInfo.setPrimitiveRestartEnable(static_cast<vk::Bool32>(value));
    }

    void setTesselationPatchControlPoints(std::uint32_t points) {
        _tesselationStateCreateInfo.patchControlPoints = points;
    }

    vk::Viewport& viewport() {
        return _viewport;
    }

    vk::Rect2D& scissor() {
        return _scissor;
    }

    vk::PipelineRasterizationStateCreateInfo& rasterizationStateCreateInfo() {
        return _rasterizationStateCreateInfo;
    }

    vk::PipelineDepthStencilStateCreateInfo& depthStencilStateCreateInfo() {
        return _depthStencilStateCreateInfo;
    }

    template<typename... Args>
    void addColorBlendAttachmentState(Args&& ... args) {
        _colorBlendAttachmentStates.emplace_back(std::forward<Args>(args)...);
    }

    void setBlendConstants(std::array<float, 4> constants) {
        _blendConstants = constants;
    }

    void setBlendLogicOp(bool logicOpEnable_, vk::LogicOp logicOp_ = vk::LogicOp::eClear) {
        _blendLogicOpEnabled = logicOpEnable_;
        _blendLogicOp = logicOp_;
    }

    void setSubpass(std::uint32_t value) {
        _subpass = value;
    }

    void addDynamicState(vk::DynamicState state) {
        _dynamicStates.push_back(state);
    }

    bool operator<(const GraphicsPipeline& rhs) const {
        using T = GraphicsPipeline;
        return DescriptorMemberComparator<&T::_stages, &T::_bindingDescriptions, &T::_vertexInputAttributeDescriptions,
                &T::_assemblyStateCreateInfo, &T::_tesselationStateCreateInfo, &T::_rasterizationStateCreateInfo,
                &T::_multisampleStateCreateInfo, &T::_depthStencilStateCreateInfo, &T::_colorBlendAttachmentStates,
                &T::_dynamicStates, &T::_blendConstants, &T::_blendLogicOpEnabled, &T::_blendLogicOp, &T::_subpass,
                &T::_viewport, &T::_scissor, &T::_pipelineLayout, &T::_renderPass>()(*this, rhs);
    }

    void setPipelineLayout(CacheReference<PipelineLayout> layout) {
        _pipelineLayout = layout;
    }

    void setRenderPass(CacheReference<RenderPass> renderPass) {
        _renderPass = renderPass;
    }

private:
    std::vector<std::tuple<vk::ShaderStageFlagBits, CacheReference<ShaderModule>, std::string>> _stages;
    std::vector<vk::VertexInputBindingDescription> _bindingDescriptions;
    std::vector<vk::VertexInputAttributeDescription> _vertexInputAttributeDescriptions;
    vk::PipelineInputAssemblyStateCreateInfo _assemblyStateCreateInfo;
    vk::PipelineTessellationStateCreateInfo _tesselationStateCreateInfo;
    vk::PipelineRasterizationStateCreateInfo _rasterizationStateCreateInfo;
    vk::PipelineMultisampleStateCreateInfo _multisampleStateCreateInfo;
    vk::PipelineDepthStencilStateCreateInfo _depthStencilStateCreateInfo;
    std::vector<vk::PipelineColorBlendAttachmentState> _colorBlendAttachmentStates;
    std::vector<vk::DynamicState> _dynamicStates;
    std::array<float, 4> _blendConstants = {{ 0, 0, 0, 0 }};
    bool _blendLogicOpEnabled = false;
    vk::LogicOp _blendLogicOp = vk::LogicOp::eClear;

    std::uint32_t _subpass { 0 };
    vk::Viewport _viewport;
    vk::Rect2D _scissor;

    CacheReference<PipelineLayout> _pipelineLayout;
    CacheReference<RenderPass> _renderPass;
};

}
