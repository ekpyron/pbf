/**
 *
 *
 * @file RenderPass.cpp
 * @brief 
 * @author clonker
 * @date 10/3/18
 */

#include "RenderPass.h"
#include "../Context.h"

namespace pbf {

objects::RenderPass::RenderPass(Context *context, const objects::RenderPass::Descriptor &desc) {
    _renderPass = context->device().createRenderPassUnique(desc.toCreateInfo());
}
}