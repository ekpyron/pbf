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

class RenderPass {
public:
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
        return MemberComparator<&T::_attachments, &T::_subpasses, &T::_subpassDependencies>()(*this, rhs);
    }

    void addSubpass(Subpass&& subpass) {
        _subpasses.emplace_back(std::move(subpass));
    }

    template<typename... Args>
    void addAttachment(Args&&... args) {
        _attachments.emplace_back(std::forward<Args>(args)...);
    }

    template<typename... Args>
    void addSubpassDependency(Args&&... args) {
        _subpassDependencies.emplace_back(std::forward<Args>(args)...);
    }

private:
    std::vector<vk::AttachmentDescription> _attachments;
    std::vector<Subpass> _subpasses;
    std::vector<vk::SubpassDependency> _subpassDependencies;
};

}
