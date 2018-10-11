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

        bool operator<(const Subpass &rhs) const {
            using T = Subpass;
            return MemberComparator<&T::flags, &T::pipelineBindPoint, &T::inputAttachments, &T::colorAttachments, &T::resolveAttachments,
                    &T::depthStencilAttachment, &T::preserveAttachments>()(*this, rhs);
        }
    };

    vk::UniqueRenderPass realize(Context *context) const;

    bool operator<(const RenderPass &rhs) const {
        using T = RenderPass;
        return MemberComparator<&T::attachments, &T::subpasses, &T::subpassDependencies>()(*this, rhs);
    }

    std::vector<vk::AttachmentDescription> attachments;
    std::vector<Subpass> subpasses;
    std::vector<vk::SubpassDependency> subpassDependencies;
};

}
