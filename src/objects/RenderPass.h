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
#include <vulkan/vulkan.hpp>
#include "DescriptorOrder.h"

namespace pbf {
namespace objects {

struct SubpassDescriptor {
    vk::SubpassDescription description;
    std::vector<vk::AttachmentReference> inputAttachments;
    std::vector<vk::AttachmentReference> colorAttachments;
    std::vector<vk::AttachmentReference> resolveAttachments;
    std::vector<vk::AttachmentReference> depthStencilAttachments;
    std::vector<std::uint32_t> preserveAttachments;

    bool operator<(const SubpassDescriptor &rhs) const {
        using T = SubpassDescriptor;
        return MemberComparator<&T::description, &T::inputAttachments, &T::colorAttachments, &T::resolveAttachments,
                                &T::depthStencilAttachments, &T::preserveAttachments>()(*this, rhs);
    }
};

class RenderPass {
public:
    struct Descriptor {
        typedef RenderPass Result;
        std::vector<vk::AttachmentDescription> attachments;
        std::vector<SubpassDescriptor> subpasses;
        std::vector<vk::SubpassDependency> subpassDependencies;
        vk::RenderPassCreateInfo createInfo;

        bool operator<(const Descriptor &rhs) const {
            using T = Descriptor;
            if(DescriptorOrder<vk::RenderPassCreateFlags>()(createInfo.flags, rhs.createInfo.flags)) return true;
            if(DescriptorOrder<vk::RenderPassCreateFlags>()(rhs.createInfo.flags, createInfo.flags)) return false;
            return MemberComparator<&T::attachments, &T::subpasses, &T::subpassDependencies>()(*this, rhs);
        }
    };

};

}
}
