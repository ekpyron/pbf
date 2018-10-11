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

using namespace pbf::objects;

vk::UniqueRenderPass RenderPass::realize(Context *context) const {
    vk::RenderPassCreateInfo createInfo({}, static_cast<uint32_t>(_attachments.size()), _attachments.data(),
                                        static_cast<uint32_t>(_subpassDescriptions.size()),
                                        _subpassDescriptions.data(),
                                        static_cast<uint32_t>(_subpassDependencies.size()),
                                        _subpassDependencies.data());
    return context->device().createRenderPassUnique(createInfo);
}
