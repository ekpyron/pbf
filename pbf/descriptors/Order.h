/**
 *
 *
 * @file DescriptorOrder.h
 * @brief 
 * @author clonker
 * @date 10/3/18
 */
#pragma once

#include <cstddef>
#include <type_traits>
#include <vector>
#include <optional>
#include <variant>

#include <vulkan/vulkan.hpp>
#include <crampl/crampl.h>

namespace pbf::descriptors {

template<typename T>
using remove_cv_ref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template<typename T, typename U>
using enable_if_same_t = std::enable_if_t<std::is_same_v<remove_cv_ref_t<T>, remove_cv_ref_t<U>>>;

template<typename T, typename = void>
struct Order;

template<typename T>
concept AllowNativeComparison = T::AllowNativeComparison ||
	std::is_same_v<T, std::string> ||
	std::is_same_v<T, std::uint32_t> ||
	std::is_same_v<T, std::int32_t> ||
	std::is_same_v<T, bool> ||
	std::is_same_v<T, float>;

template<AllowNativeComparison T>
struct Order<T> {
	bool operator()(const T &lhs, const T &rhs) const {
		return lhs < rhs;
	}
};

template<typename T>
struct Order<T, std::void_t<typename T::Compare>> {
    bool operator()(const T &lhs, const T &rhs) const {
        return typename T::Compare()(lhs, rhs);
    }
};

template<typename T, size_t N>
struct Order<std::array<T, N>> {

	bool operator()(const std::array<T, N> &lhs, const std::array<T, N> &rhs) const {
		Order<remove_cv_ref_t<T>> compare;
		return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), compare);
	}
};

template<typename T>
struct Order<std::optional<T>> {

    bool operator()(const std::optional<T> &lhs, const std::optional<T> &rhs) const {
        if(!rhs) return false;
        if(!lhs) return true;
        return Order<T>()(lhs.value(), rhs.value());
    }
};

template<typename T>
struct Order<std::vector<T>> {

    bool operator()(const std::vector<T> &lhs, const std::vector<T> &rhs) const {
        Order<remove_cv_ref_t<T>> compare;
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), compare);
    }
};

template<typename... Args>
struct Order<std::variant<Args...>> {

	bool operator()(const std::variant<Args...> &lhs, const std::variant<Args...> &rhs) const {
		return std::visit([](auto const& lhsTyped, auto const& rhsTyped) {
			if constexpr (std::is_same_v<decltype(lhsTyped), decltype(rhsTyped)>) {
				return Order<remove_cv_ref_t<decltype(lhsTyped)>>{}(lhsTyped, rhsTyped);
			} else {
				return false;
			}
		}, lhs, rhs);
	}
};

template<typename... Args>
struct Order<std::tuple<Args...>> {

    bool operator()(const std::tuple<Args...> &lhs, const std::tuple<Args...> &rhs) const {
        return TupleHelper<0>()(lhs, rhs);
    }
private:
    template<std::size_t N>
    struct TupleHelper {
        using tup = std::tuple<Args...>;

        bool operator()(const tup& lhs, const tup &rhs) const {
            if constexpr (N < sizeof...(Args)) {
                Order<std::tuple_element_t<N, tup>> cmp;
                if (cmp(std::get<N>(lhs), std::get<N>(rhs)))
                    return true;
                if (cmp(std::get<N>(rhs), std::get<N>(lhs)))
                    return false;

                return TupleHelper<N + 1>()(lhs, rhs);
            } else {
                return false;
            }
        }
    };

};

template<typename T>
struct Order<T, std::enable_if_t<std::is_enum_v<T>>> {
    bool operator()(const T& lhs, const T& rhs) const {
        return static_cast<std::underlying_type_t<T>>(lhs) < static_cast<std::underlying_type_t<T>>(rhs);
    }
};


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


/*
template<typename T>
struct Order<T, enable_if_same_t<T, vk::AttachmentReference>>
        : PBFMemberComparator<&T::attachment, &T::layout> {};
*/

template<typename T, typename U>
concept identity_concept = std::is_same_v<T, U>;

template<identity_concept<vk::AttachmentReference> T>
struct Order<T>
	: PBFMemberComparator<&T::attachment, &T::layout> {};

template<identity_concept<vk::AttachmentDescription> T>
struct Order<T>
        : PBFMemberComparator<&T::flags, &T::format, &T::samples, &T::loadOp, &T::storeOp, &T::stencilLoadOp,
                           &T::stencilStoreOp, &T::initialLayout, &T::finalLayout> {};

template<identity_concept<vk::SubpassDependency> T>
struct Order<T>
        : PBFMemberComparator<&T::srcSubpass, &T::dstSubpass, &T::srcStageMask, &T::dstStageMask,
                           &T::srcAccessMask, &T::dstAccessMask, &T::dependencyFlags> {};

template<identity_concept<vk::PipelineInputAssemblyStateCreateInfo> T>
struct Order<T>
        : PBFMemberComparator<&T::topology, &T::primitiveRestartEnable> {};

template<identity_concept<vk::VertexInputBindingDescription> T>
struct Order<T>
        : PBFMemberComparator<&T::binding, &T::stride, &T::inputRate> {};

template<identity_concept<vk::PipelineTessellationStateCreateInfo> T>
struct Order<T>
        : PBFMemberComparator<&T::patchControlPoints> {};

template<identity_concept<vk::VertexInputAttributeDescription> T>
struct Order<T>
        : PBFMemberComparator<&T::location, &T::binding, &T::format, &T::offset> {};

template<identity_concept<vk::PipelineRasterizationStateCreateInfo> T>
struct Order<T>
        : PBFMemberComparator<&T::depthClampEnable, &T::rasterizerDiscardEnable, &T::polygonMode, &T::cullMode,
                           &T::frontFace, &T::depthBiasEnable, &T::depthBiasConstantFactor, &T::depthBiasClamp,
                           &T::depthBiasSlopeFactor, &T::lineWidth> {};

// todo implement this at some point
template<identity_concept<vk::PipelineMultisampleStateCreateInfo> T>
struct Order<T>
{ bool operator()(...) const { return false; } };

template<identity_concept<vk::PipelineDepthStencilStateCreateInfo> T>
struct Order<T>
        : PBFMemberComparator<&T::depthTestEnable, &T::depthWriteEnable, &T::depthCompareOp, &T::depthBoundsTestEnable,
                           &T::stencilTestEnable, &T::front, &T::back, &T::minDepthBounds, &T::maxDepthBounds> {};

template<identity_concept<vk::PipelineColorBlendAttachmentState> T>
struct Order<T>
        : PBFMemberComparator<&T::blendEnable, &T::srcColorBlendFactor, &T::dstColorBlendFactor, &T::colorBlendOp,
                           &T::srcAlphaBlendFactor, &T::dstAlphaBlendFactor, &T::alphaBlendOp, &T::colorWriteMask> {};

template<identity_concept<vk::StencilOpState> T>
struct Order<T>
        : PBFMemberComparator<&T::failOp, &T::passOp, &T::depthFailOp, &T::compareOp, &T::compareMask,
                           &T::writeMask, &T::reference> {};

template<identity_concept<vk::Viewport> T>
struct Order<T> : PBFMemberComparator<&T::x, &T::y, &T::width, &T::height, &T::minDepth, &T::maxDepth> {};

template<identity_concept<vk::Rect2D> T>
struct Order<T> : PBFMemberComparator<&T::offset, &T::extent> {};

template<identity_concept<vk::Offset2D> T>
struct Order<T> : PBFMemberComparator<&T::x, &T::y> {};

template<identity_concept<vk::Extent2D> T>
struct Order<T> : PBFMemberComparator<&T::width, &T::height> {};

template<identity_concept<vk::PushConstantRange> T>
struct Order<T> : PBFMemberComparator<&T::stageFlags, &T::offset, &T::size> {};


}
