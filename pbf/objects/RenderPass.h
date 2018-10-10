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
#include <pbf/objects/DescriptorOrder.h>

namespace pbf::objects {

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
    class Descriptor {
    public:
        typedef RenderPass Result;

        Descriptor(const vk::RenderPassCreateFlags &flags) : _flags(flags) {}

        Descriptor (const Descriptor&) = delete;
        Descriptor &operator=(const Descriptor &) = delete;
        Descriptor (Descriptor &&) = default;
        Descriptor &operator=(Descriptor &&) = default;
        ~Descriptor() = default;


        bool operator<(const Descriptor &rhs) const {
            using T = Descriptor;
            return DescriptorMemberComparator<&T::_flags, &T::_attachments, &T::_subpasses, &T::_subpassDependencies>()(
                    *this,rhs
            );
        }

        auto toCreateInfo() const {
            return vk::RenderPassCreateInfo(_flags, static_cast<uint32_t>(_attachments.size()), _attachments.data(),
                                            static_cast<uint32_t>(_subpassDescriptions.size()),
                                            _subpassDescriptions.data(),
                                            static_cast<uint32_t>(_subpassDependencies.size()),
                                            _subpassDependencies.data());
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
        vk::RenderPassCreateFlags _flags;
        std::vector<vk::AttachmentDescription> _attachments;
        std::vector<vk::SubpassDescription> _subpassDescriptions;
        std::vector<SubpassDescriptor> _subpasses;
        std::vector<vk::SubpassDependency> _subpassDependencies;
    };

    RenderPass(Context *context, const Descriptor &desc);

    const vk::RenderPass &get() const {
        return *_renderPass;
    }
private:

    vk::UniqueRenderPass _renderPass;

};

}
