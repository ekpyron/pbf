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
#include <pbf/descriptors/Order.h>

namespace pbf::descriptors {

template<auto... M>
using MemberComparator = crampl::MemberComparatorTemplate<Order, M...>;

template<typename BitType, typename MaskType>
struct Order<vk::Flags<BitType, MaskType>> {
    bool operator()(const vk::Flags<BitType, MaskType>& lhs, const vk::Flags<BitType, MaskType> &rhs) const {
        return static_cast<MaskType>(lhs) < static_cast<MaskType>(rhs);
    }
};

template<auto PtrMember, auto SizeMember>
constexpr auto PointerRangeCompare = crampl::PointerRangeCompare<Order, PtrMember, SizeMember>();

template<typename T>
struct Order<T, enable_if_same_t<T, vk::SubpassDescription>>
    : MemberComparator<&T::flags, &T::pipelineBindPoint,
                       PointerRangeCompare<&T::pInputAttachments, &T::inputAttachmentCount>,
                       PointerRangeCompare<&T::pColorAttachments, &T::colorAttachmentCount>,
                       PointerRangeCompare<&T::pResolveAttachments, &T::colorAttachmentCount>,
                       PointerRangeCompare<&T::pDepthStencilAttachment, 1>,
                       PointerRangeCompare<&T::pPreserveAttachments, &T::preserveAttachmentCount>> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::AttachmentReference>>
        : MemberComparator<&T::attachment, &T::layout> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::AttachmentDescription>>
        : MemberComparator<&T::flags, &T::format, &T::samples, &T::loadOp, &T::storeOp, &T::stencilLoadOp,
                           &T::stencilStoreOp, &T::initialLayout, &T::finalLayout> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::SubpassDependency>>
        : MemberComparator<&T::srcSubpass, &T::dstSubpass, &T::srcStageMask, &T::dstStageMask,
                           &T::srcAccessMask, &T::dstAccessMask, &T::dependencyFlags> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::PipelineInputAssemblyStateCreateInfo>>
        : MemberComparator<&T::topology, &T::primitiveRestartEnable> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::VertexInputBindingDescription>>
        : MemberComparator<&T::binding, &T::stride, &T::inputRate> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::PipelineTessellationStateCreateInfo>>
        : MemberComparator<&T::patchControlPoints> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::VertexInputAttributeDescription>>
        : MemberComparator<&T::location, &T::binding, &T::format, &T::offset> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::PipelineRasterizationStateCreateInfo>>
        : MemberComparator<&T::depthClampEnable, &T::rasterizerDiscardEnable, &T::polygonMode, &T::cullMode,
                           &T::frontFace, &T::depthBiasEnable, &T::depthBiasConstantFactor, &T::depthBiasClamp,
                           &T::depthBiasSlopeFactor, &T::lineWidth> {};

// todo implement this at some point
template<typename T>
struct Order<T, enable_if_same_t<T, vk::PipelineMultisampleStateCreateInfo>>
{ bool operator()(const T &lhs, const T &rhs) const { return false; } };

template<typename T>
struct Order<T, enable_if_same_t<T, vk::PipelineDepthStencilStateCreateInfo>>
        : MemberComparator<&T::depthTestEnable, &T::depthWriteEnable, &T::depthCompareOp, &T::depthBoundsTestEnable,
                           &T::stencilTestEnable, &T::front, &T::back, &T::minDepthBounds, &T::maxDepthBounds> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::PipelineColorBlendAttachmentState>>
        : MemberComparator<&T::blendEnable, &T::srcColorBlendFactor, &T::dstColorBlendFactor, &T::colorBlendOp,
                           &T::srcAlphaBlendFactor, &T::dstAlphaBlendFactor, &T::alphaBlendOp, &T::colorWriteMask> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::StencilOpState>>
        : MemberComparator<&T::failOp, &T::passOp, &T::depthFailOp, &T::compareOp, &T::compareMask,
                           &T::writeMask, &T::reference> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::Viewport>>
        : MemberComparator<&T::x, &T::y, &T::width, &T::height, &T::minDepth, &T::maxDepth> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::Rect2D>>
        : MemberComparator<&T::offset, &T::extent> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::Offset2D>>
        : MemberComparator<&T::x, &T::y> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::Extent2D>>
        : MemberComparator<&T::width, &T::height> {};
}
