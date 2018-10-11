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

using namespace pbf::descriptors;

vk::UniqueRenderPass RenderPass::realize(Context *context) const {
    std::vector<vk::SubpassDescription> subpassDescriptions;

    for (auto const& subpass: _subpasses)
    {
        subpassDescriptions.emplace_back(subpass.flags, subpass.pipelineBindPoint, static_cast<uint32_t>(subpass.inputAttachments.size()),
                                          subpass.inputAttachments.data(), static_cast<uint32_t>(subpass.colorAttachments.size()),
                                          subpass.colorAttachments.data(),
                                          subpass.resolveAttachments.empty() ? nullptr : subpass.resolveAttachments.data(),
                                          subpass.depthStencilAttachment ? &*subpass.depthStencilAttachment : nullptr,
                                          static_cast<uint32_t>(subpass.preserveAttachments.size()), subpass.preserveAttachments.data());

    }

    vk::RenderPassCreateInfo createInfo({}, static_cast<uint32_t>(_attachments.size()), _attachments.data(),
                                        static_cast<uint32_t>(subpassDescriptions.size()),
                                        subpassDescriptions.data(),
                                        static_cast<uint32_t>(_subpassDependencies.size()),
                                        _subpassDependencies.data());
    return context->device().createRenderPassUnique(createInfo);
}
