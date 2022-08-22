/**
 *
 *
 * @file RenderPass.h
 * @brief 
 * @author clonker
 * @date 10/3/18
 */
#pragma once

#include <vector>
#include <optional>

#include <vulkan/vulkan.hpp>

#include <pbf/common.h>
#include <pbf/descriptors/Order.h>

namespace pbf::descriptors {

struct RenderPass {
    struct Subpass {
        vk::SubpassDescriptionFlags flags;
        vk::PipelineBindPoint pipelineBindPoint;
        std::vector<vk::AttachmentReference> inputAttachments;
        std::vector<vk::AttachmentReference> colorAttachments;
        std::vector<vk::AttachmentReference> resolveAttachments;
        std::optional<vk::AttachmentReference> depthStencilAttachment;
        std::vector<std::uint32_t> preserveAttachments;

	private:
		using T = Subpass;
	public:
        using Compare = PBFMemberComparator<&T::flags, &T::pipelineBindPoint, &T::inputAttachments, &T::colorAttachments, &T::resolveAttachments,
                    &T::depthStencilAttachment, &T::preserveAttachments>;
    };

    vk::UniqueRenderPass realize(ContextInterface &context) const;

    std::vector<vk::AttachmentDescription> attachments;
    std::vector<Subpass> subpasses;
    std::vector<vk::SubpassDependency> subpassDependencies;

#ifndef NDEBUG
    std::string debugName;
#endif

private:
	using T = RenderPass;
public:
	using Compare = PBFMemberComparator<&T::attachments, &T::subpasses, &T::subpassDependencies>;

};

}
