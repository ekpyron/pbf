/**
 *
 *
 * @file VkOrder.h
 * @brief 
 * @author clonker
 * @date 10/3/18
 */
#pragma once

#include <vulkan/vulkan.hpp>
#include "DescriptorOrder.h"

namespace pbf {

template<typename T>
struct DescriptorOrder;

template<typename BitType, typename MaskType>
struct DescriptorOrder<vk::Flags<BitType, MaskType>> {
    bool operator()(const vk::Flags<BitType, MaskType>& lhs, const vk::Flags<BitType, MaskType> &rhs) const {
        return static_cast<MaskType>(lhs) < static_cast<MaskType>(rhs);
    }
};

template<> struct DescriptorOrder<vk::SubpassDescription> {
    using T = vk::SubpassDescription;
    bool operator()(const T& lhs, const T& rhs) const {
        return MemberComparator<&T::flags, &T::pipelineBindPoint,
                                AutoList<&T::pInputAttachments, &T::inputAttachmentCount>,
                                AutoList<&T::pColorAttachments, &T::colorAttachmentCount>,
                                AutoList<&T::pResolveAttachments, &T::colorAttachmentCount>,
                                AutoList<&T::pDepthStencilAttachment, 1>,
                                AutoList<&T::pPreserveAttachments, &T::preserveAttachmentCount>>()(lhs, rhs);
    }
};

template<> struct DescriptorOrder<vk::ImageLayout> : EnumDescriptorOrder<vk::ImageLayout>{};

template<> struct DescriptorOrder<vk::AttachmentReference> {
    using T = vk::AttachmentReference;
    bool operator()(const T& lhs, const T&rhs) const {
        return MemberComparator<&T::attachment, &T::layout>()(lhs, rhs);
    }
};

template<> struct DescriptorOrder<vk::AttachmentDescription> {
    using T = vk::AttachmentDescription;
    bool operator()(const T &lhs, const T &rhs) const {
        return MemberComparator<&T::flags, &T::format, &T::samples, &T::loadOp, &T::storeOp, &T::stencilLoadOp,
                                &T::stencilStoreOp, &T::initialLayout, &T::finalLayout>()(lhs, rhs);
    }
};

template<> struct DescriptorOrder<vk::SubpassDependency> {
    using T = vk::SubpassDependency;
    bool operator()(const T &lhs, const T &rhs) const {
        return MemberComparator<&T::srcSubpass, &T::dstSubpass, &T::srcStageMask, &T::dstStageMask,
                                &T::srcAccessMask, &T::dstAccessMask, &T::dependencyFlags>()(lhs, rhs);
    }
};

}
