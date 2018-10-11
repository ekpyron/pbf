/**
 *
 *
 * @file PipelineLayout.cpp
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#include "PipelineLayout.h"
#include <pbf/Context.h>

using namespace pbf::objects;

vk::UniquePipelineLayout PipelineLayout::realize(Context *context) const {
    const auto &device = context->device();
    return device.createPipelineLayoutUnique({
            {}, 0, nullptr, 0, nullptr
    });
}
