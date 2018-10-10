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
#include <crampl/crampl.h>
#include <pbf/objects/DescriptorOrder.h>

namespace pbf {

template<auto... M>
using DescriptorMemberComparator = crampl::MemberComparatorTemplate<DescriptorOrder, M...>;

template<typename BitType, typename MaskType>
struct DescriptorOrder<vk::Flags<BitType, MaskType>> {
    bool operator()(const vk::Flags<BitType, MaskType>& lhs, const vk::Flags<BitType, MaskType> &rhs) const {
        return static_cast<MaskType>(lhs) < static_cast<MaskType>(rhs);
    }
};

template<auto PtrMember, auto SizeMember>
constexpr auto DescriptorPointerRangeCompare = crampl::PointerRangeCompare<DescriptorOrder, PtrMember, SizeMember>();

template<typename T>
struct DescriptorOrder<T, enable_if_same_t<T, vk::SubpassDescription>>
    : DescriptorMemberComparator<&T::flags, &T::pipelineBindPoint,
                                 DescriptorPointerRangeCompare<&T::pInputAttachments, &T::inputAttachmentCount>,
                                 DescriptorPointerRangeCompare<&T::pColorAttachments, &T::colorAttachmentCount>,
                                 DescriptorPointerRangeCompare<&T::pResolveAttachments, &T::colorAttachmentCount>,
                                 DescriptorPointerRangeCompare<&T::pDepthStencilAttachment, 1>,
                                 DescriptorPointerRangeCompare<&T::pPreserveAttachments, &T::preserveAttachmentCount>> {};

template<typename T>
struct DescriptorOrder<T, enable_if_same_t<T, vk::AttachmentReference>>
        : DescriptorMemberComparator<&T::attachment, &T::layout> {};

template<typename T>
struct DescriptorOrder<T, enable_if_same_t<T, vk::AttachmentDescription>>
        : DescriptorMemberComparator<&T::flags, &T::format, &T::samples, &T::loadOp, &T::storeOp, &T::stencilLoadOp,
                                     &T::stencilStoreOp, &T::initialLayout, &T::finalLayout> {};

template<typename T>
struct DescriptorOrder<T, enable_if_same_t<T, vk::SubpassDependency>>
        : DescriptorMemberComparator<&T::srcSubpass, &T::dstSubpass, &T::srcStageMask, &T::dstStageMask,
        &T::srcAccessMask, &T::dstAccessMask, &T::dependencyFlags> {};

template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::PipelineInputAssemblyStateCreateInfo>>
        : DescriptorMemberComparator<&T::topology, &T::primitiveRestartEnable> {};

template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::VertexInputBindingDescription>>
        : DescriptorMemberComparator<&T::binding, &T::stride, &T::inputRate> {};

template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::PipelineTessellationStateCreateInfo>>
        : DescriptorMemberComparator<&T::patchControlPoints> {};

template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::VertexInputAttributeDescription>>
        : DescriptorMemberComparator<&T::location, &T::binding, &T::format, &T::offset> {};

template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::PipelineRasterizationStateCreateInfo>>
        : DescriptorMemberComparator<&T::depthClampEnable, &T::rasterizerDiscardEnable, &T::polygonMode, &T::cullMode,
            &T::frontFace, &T::depthBiasEnable, &T::depthBiasConstantFactor, &T::depthBiasClamp,
            &T::depthBiasSlopeFactor, &T::lineWidth> {};

// todo implement this at some point
template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::PipelineMultisampleStateCreateInfo>>
        { bool operator()(const T &lhs, const T &rhs) const { return false; } };

template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::PipelineDepthStencilStateCreateInfo>>
        : DescriptorMemberComparator<&T::depthTestEnable, &T::depthWriteEnable, &T::depthCompareOp, &T::depthBoundsTestEnable,
                             &T::stencilTestEnable, &T::front, &T::back, &T::minDepthBounds, &T::maxDepthBounds> {};

template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::PipelineColorBlendAttachmentState>>
        : DescriptorMemberComparator<&T::blendEnable, &T::srcColorBlendFactor, &T::dstColorBlendFactor, &T::colorBlendOp,
                             &T::srcAlphaBlendFactor, &T::dstAlphaBlendFactor, &T::alphaBlendOp, &T::colorWriteMask> {};

template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::StencilOpState>>
        : DescriptorMemberComparator<&T::failOp, &T::passOp, &T::depthFailOp, &T::compareOp, &T::compareMask,
                             &T::writeMask, &T::reference> {};

template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::Viewport>>
        : DescriptorMemberComparator<&T::x, &T::y, &T::width, &T::height, &T::minDepth, &T::maxDepth> {};

template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::Rect2D>>
        : DescriptorMemberComparator<&T::offset, &T::extent> {};

template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::Offset2D>>
        : DescriptorMemberComparator<&T::x, &T::y> {};

template<typename T> struct DescriptorOrder<T, enable_if_same_t<T, vk::Extent2D>>
        : DescriptorMemberComparator<&T::width, &T::height> {};
}
