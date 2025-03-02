#pragma once
#include <pbf/common.h>
#include <pbf/Cache.h>
#include <pbf/descriptors/PipelineLayout.h>

namespace pbf
{
struct Pipeline
{
    vk::UniquePipeline pipeline;
    CacheReference<descriptors::PipelineLayout> pipelineLayout;
    explicit operator bool() const { return !!pipeline; }
    bool operator!() const { return !pipeline; }
    Pipeline const& operator*() const { return *this; }
    void reset() { pipeline.reset(); }

    static CacheReference<descriptors::PipelineLayout> deducePipelineLayout(Cache& _cache, std::vector<descriptors::ShaderStage> const& _shaderStages
#ifndef NDEBUG
    , std::string const& _debugName
#endif
        )
    {
        descriptors::PipelineLayout pipelineLayout;
#ifndef NDEBUG
        pipelineLayout.debugName = _debugName + " Pipeline Layout";
#endif
        std::vector<uint32_t> used;
        auto setIthLayout = [&](uint32_t i, auto _layout)
        {
            if (pipelineLayout.setLayouts.size() <= i)
            {
                pipelineLayout.setLayouts.resize(i + 1);
                used.resize(i + 1);
            }
            if (used[i])
                pipelineLayout.setLayouts[i] = _cache.fetch(pipelineLayout.setLayouts[i].descriptor() + _layout);
            else
                pipelineLayout.setLayouts[i] = _cache.fetch(_layout);
            used[i] = true;
        };

        for (auto x:used) assert(x);

        for (auto& module: _shaderStages)
            for (auto& descriptorSetInfo: module.module->descriptorSetInfos)
                setIthLayout(descriptorSetInfo.set, descriptorSetInfo.descriptorSetLayout);

        for (auto const& stage: _shaderStages)
            for (auto& pushConstant: stage.module->pushConstantInfos)
                pipelineLayout.pushConstants.emplace_back(vk::ShaderStageFlagBits::eAll, pushConstant.offset, pushConstant.size);

        return _cache.fetch(pipelineLayout);
    }
};
}