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
    device.createGraphicsPipelineUnique(nullptr, )
}

}

