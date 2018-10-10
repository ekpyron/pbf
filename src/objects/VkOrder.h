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
#include <crampl/crampl.h>

namespace pbf {

template<typename T>
struct DescriptorOrder;

template<auto... M>
using DescriptorMemberComparator = crampl::MemberComparatorTemplate <DescriptorOrder, M...>;

template<typename BitType, typename MaskType>
struct DescriptorOrder<vk::Flags<BitType, MaskType>> {
    bool operator()(const vk::Flags<BitType, MaskType>& lhs, const vk::Flags<BitType, MaskType> &rhs) const {
        return static_cast<MaskType>(lhs) < static_cast<MaskType>(rhs);
    }
};

template<auto PtrMember, auto SizeMember>
constexpr auto DescriptorPointerRangeCompare() {
    return crampl::PointerRangeCompare<DescriptorOrder, PtrMember, SizeMember>();
}

template<> struct DescriptorOrder<vk::SubpassDescription> {
    using T = vk::SubpassDescription;
    bool operator()(const T& lhs, const T& rhs) const {
        return DescriptorMemberComparator<&T::flags, &T::pipelineBindPoint,
                DescriptorPointerRangeCompare<&T::pInputAttachments, &T::inputAttachmentCount>(),
                DescriptorPointerRangeCompare<&T::pColorAttachments, &T::colorAttachmentCount>(),
                DescriptorPointerRangeCompare<&T::pResolveAttachments, &T::colorAttachmentCount>(),
                DescriptorPointerRangeCompare<&T::pDepthStencilAttachment, 1>(),
                DescriptorPointerRangeCompare<&T::pPreserveAttachments, &T::preserveAttachmentCount>()>()(lhs, rhs);
    }
};

template<> struct DescriptorOrder<vk::ImageLayout> : EnumDescriptorOrder<vk::ImageLayout>{};

template<> struct DescriptorOrder<vk::AttachmentReference> {
    using T = vk::AttachmentReference;
    bool operator()(const T& lhs, const T&rhs) const {
        return DescriptorMemberComparator<&T::attachment, &T::layout>()(lhs, rhs);
    }
};

template<> struct DescriptorOrder<vk::AttachmentDescription> {
    using T = vk::AttachmentDescription;
    bool operator()(const T &lhs, const T &rhs) const {
        return DescriptorMemberComparator<&T::flags, &T::format, &T::samples, &T::loadOp, &T::storeOp, &T::stencilLoadOp,
                                        &T::stencilStoreOp, &T::initialLayout, &T::finalLayout>()(lhs, rhs);
    }
};

template<> struct DescriptorOrder<vk::SubpassDependency> {
    using T = vk::SubpassDependency;
    bool operator()(const T &lhs, const T &rhs) const {
        return DescriptorMemberComparator<&T::srcSubpass, &T::dstSubpass, &T::srcStageMask, &T::dstStageMask,
                                        &T::srcAccessMask, &T::dstAccessMask, &T::dependencyFlags>()(lhs, rhs);
    }
};

}
