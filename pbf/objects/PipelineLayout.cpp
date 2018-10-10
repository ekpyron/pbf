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

using namespace pbf;
using namespace objects;

PipelineLayout::PipelineLayout(Context *context, const PipelineLayout::Descriptor &descriptor) {
    const auto &device = context->device();
    _layout = device.createPipelineLayoutUnique({
            {}, 0, nullptr, 0, nullptr
    });
}
