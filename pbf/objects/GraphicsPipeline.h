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
#include <vulkan/vulkan.hpp>

namespace pbf::objects {

class GraphicsPipeline {
public:

    class Descriptor {
    public:
        using Result = GraphicsPipeline;
        Descriptor();

        auto toDescription() const {
            return vk::GraphicsPipelineCreateInfo({}, )
        }
    };

    GraphicsPipeline(Context *context, const Descriptor &descriptor);
};

}
