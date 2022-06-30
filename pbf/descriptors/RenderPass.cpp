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

    for (auto const& subpass: subpasses)
    {
		subpassDescriptions.emplace_back(vk::SubpassDescription{
			.flags = subpass.flags,
			.pipelineBindPoint = subpass.pipelineBindPoint,
			.inputAttachmentCount = static_cast<uint32_t>(subpass.inputAttachments.size()),
			.pInputAttachments = subpass.inputAttachments.data(),
			.colorAttachmentCount = static_cast<uint32_t>(subpass.colorAttachments.size()),
			.pColorAttachments = subpass.colorAttachments.data(),
			.pResolveAttachments = subpass.resolveAttachments.empty() ? nullptr : subpass.resolveAttachments.data(),
			.pDepthStencilAttachment = subpass.depthStencilAttachment ? &*subpass.depthStencilAttachment : nullptr,
			.preserveAttachmentCount = static_cast<uint32_t>(subpass.preserveAttachments.size()),
			.pPreserveAttachments = subpass.preserveAttachments.data()
		});
    }

    vk::RenderPassCreateInfo createInfo{
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = static_cast<uint32_t>(subpassDescriptions.size()),
		.pSubpasses = subpassDescriptions.data(),
		.dependencyCount = static_cast<uint32_t>(subpassDependencies.size()),
		.pDependencies = subpassDependencies.data()
	};
    return context->device().createRenderPassUnique(createInfo);
}
