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
#include <pbf/descriptors/DescriptorOrder.h>

namespace pbf::descriptors {

struct SubpassDescriptor {
    vk::SubpassDescriptionFlags flags;
    vk::PipelineBindPoint pipelineBindPoint;
    std::vector<vk::AttachmentReference> inputAttachments;
    std::vector<vk::AttachmentReference> colorAttachments;
    std::vector<vk::AttachmentReference> resolveAttachments;
    std::optional<vk::AttachmentReference> depthStencilAttachment;
    std::vector<std::uint32_t> preserveAttachments;

    auto toDescription() const {
        return vk::SubpassDescription(flags, pipelineBindPoint, static_cast<uint32_t>(inputAttachments.size()),
                                      inputAttachments.data(), static_cast<uint32_t>(colorAttachments.size()),
                                      colorAttachments.data(),
                                      resolveAttachments.empty() ? nullptr : resolveAttachments.data(),
                                      depthStencilAttachment ? &*depthStencilAttachment : nullptr,
                                      static_cast<uint32_t>(preserveAttachments.size()), preserveAttachments.data());
    }

    bool operator<(const SubpassDescriptor &rhs) const {
        using T = SubpassDescriptor;
        return DescriptorMemberComparator<&T::flags, &T::pipelineBindPoint, &T::inputAttachments, &T::colorAttachments, &T::resolveAttachments,
                &T::depthStencilAttachment, &T::preserveAttachments>()(*this, rhs);
    }
};

class RenderPass {
public:
    vk::UniqueRenderPass realize(Context *context) const;

    bool operator<(const RenderPass &rhs) const {
        using T = RenderPass;
        return DescriptorMemberComparator<&T::_attachments, &T::_subpasses, &T::_subpassDependencies>()(*this, rhs);
    }

    template<typename ... Args>
    void addSubpass(Args&&... args) {
        _subpasses.emplace_back(std::forward<Args>(args)...);
        _subpassDescriptions.push_back(_subpasses.back().toDescription());
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
    std::vector<vk::SubpassDescription> _subpassDescriptions;
    std::vector<SubpassDescriptor> _subpasses;
    std::vector<vk::SubpassDependency> _subpassDependencies;
};

}
