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
using PBFMemberComparator = crampl::MemberComparatorTemplate<Order, M...>;

template<typename BitType>
struct Order<vk::Flags<BitType>> {
    bool operator()(const vk::Flags<BitType>& lhs, const vk::Flags<BitType> &rhs) const {
		return lhs < rhs;
        //return static_cast<typename BitType::MaskType>(lhs) < static_cast<typename BitType::MaskType>(rhs);
    }
};

template<auto PtrMember, auto SizeMember>
constexpr auto PointerRangeCompare = crampl::PointerRangeCompare<Order, PtrMember, SizeMember>();

template<typename T>
struct Order<T, enable_if_same_t<T, vk::SubpassDescription>>
    : PBFMemberComparator<&T::flags, &T::pipelineBindPoint,
                       PointerRangeCompare<&T::pInputAttachments, &T::inputAttachmentCount>,
                       PointerRangeCompare<&T::pColorAttachments, &T::colorAttachmentCount>,
                       PointerRangeCompare<&T::pResolveAttachments, &T::colorAttachmentCount>,
                       PointerRangeCompare<&T::pDepthStencilAttachment, 1>,
                       PointerRangeCompare<&T::pPreserveAttachments, &T::preserveAttachmentCount>> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::AttachmentReference>>
        : PBFMemberComparator<&T::attachment, &T::layout> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::AttachmentDescription>>
        : PBFMemberComparator<&T::flags, &T::format, &T::samples, &T::loadOp, &T::storeOp, &T::stencilLoadOp,
                           &T::stencilStoreOp, &T::initialLayout, &T::finalLayout> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::SubpassDependency>>
        : PBFMemberComparator<&T::srcSubpass, &T::dstSubpass, &T::srcStageMask, &T::dstStageMask,
                           &T::srcAccessMask, &T::dstAccessMask, &T::dependencyFlags> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::PipelineInputAssemblyStateCreateInfo>>
        : PBFMemberComparator<&T::topology, &T::primitiveRestartEnable> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::VertexInputBindingDescription>>
        : PBFMemberComparator<&T::binding, &T::stride, &T::inputRate> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::PipelineTessellationStateCreateInfo>>
        : PBFMemberComparator<&T::patchControlPoints> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::VertexInputAttributeDescription>>
        : PBFMemberComparator<&T::location, &T::binding, &T::format, &T::offset> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::PipelineRasterizationStateCreateInfo>>
        : PBFMemberComparator<&T::depthClampEnable, &T::rasterizerDiscardEnable, &T::polygonMode, &T::cullMode,
                           &T::frontFace, &T::depthBiasEnable, &T::depthBiasConstantFactor, &T::depthBiasClamp,
                           &T::depthBiasSlopeFactor, &T::lineWidth> {};

// todo implement this at some point
template<typename T>
struct Order<T, enable_if_same_t<T, vk::PipelineMultisampleStateCreateInfo>>
{ bool operator()(...) const { return false; } };

template<typename T>
struct Order<T, enable_if_same_t<T, vk::PipelineDepthStencilStateCreateInfo>>
        : PBFMemberComparator<&T::depthTestEnable, &T::depthWriteEnable, &T::depthCompareOp, &T::depthBoundsTestEnable,
                           &T::stencilTestEnable, &T::front, &T::back, &T::minDepthBounds, &T::maxDepthBounds> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::PipelineColorBlendAttachmentState>>
        : PBFMemberComparator<&T::blendEnable, &T::srcColorBlendFactor, &T::dstColorBlendFactor, &T::colorBlendOp,
                           &T::srcAlphaBlendFactor, &T::dstAlphaBlendFactor, &T::alphaBlendOp, &T::colorWriteMask> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::StencilOpState>>
        : PBFMemberComparator<&T::failOp, &T::passOp, &T::depthFailOp, &T::compareOp, &T::compareMask,
                           &T::writeMask, &T::reference> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::Viewport>>
        : PBFMemberComparator<&T::x, &T::y, &T::width, &T::height, &T::minDepth, &T::maxDepth> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::Rect2D>>
        : PBFMemberComparator<&T::offset, &T::extent> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::Offset2D>>
        : PBFMemberComparator<&T::x, &T::y> {};

template<typename T>
struct Order<T, enable_if_same_t<T, vk::Extent2D>>
        : PBFMemberComparator<&T::width, &T::height> {};
template<typename T>
struct Order<T, enable_if_same_t<T, vk::PushConstantRange>> : PBFMemberComparator<&T::stageFlags, &T::offset, &T::size> {};

} // namespace pbf::descriptors
