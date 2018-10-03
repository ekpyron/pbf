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

namespace pbf {


template<typename T>
struct DescriptorOrder;


template<typename T, typename size_type = std::uint32_t>
struct PointerRangeView {
    PointerRangeView(const T* ptr_, size_type size_) : ptr(ptr_), size(size_) {}
    bool operator<(const PointerRangeView<T, size_type> &rhs) const {
        if(ptr == nullptr) {
            return rhs.ptr != nullptr;
        }
        if(rhs.ptr == nullptr) return false;
        if (size < rhs.size) return true;
        if (rhs.size < size) return false;
        for (size_type i = 0; i < size; i++) {
            DescriptorOrder<T> compare;
            if (compare(ptr[i], rhs.ptr[i])) return true;
            if (compare(rhs.ptr[i], ptr[i])) return false;
        }
        return false;
    }
    const T* ptr;
    size_type size;
};

namespace detail {
template<auto...>
struct AutoListType {};
}

template<auto M1, auto M2>
constexpr detail::AutoListType<M1,M2>* AutoList = nullptr;

template<auto... M>
struct MemberComparator;

template<auto M1, auto... M>
struct MemberComparator<M1, M...> {

    template<typename A, typename N, typename T>
    bool compare(const A& lhs, const A& rhs, T N::*m) const {
        return DescriptorOrder<T>()(lhs.*m, rhs.*m);
    }
    template<typename A, auto ptrMember, auto sizeMember>
    bool compare(const A& lhs, const A& rhs, detail::AutoListType<ptrMember, sizeMember>*) const {
        if constexpr (std::is_integral_v<decltype(sizeMember)>) {
            return PointerRangeView(lhs.*ptrMember, sizeMember) < PointerRangeView(rhs.*ptrMember, sizeMember);
        } else {
            return PointerRangeView(lhs.*ptrMember, lhs.*sizeMember) < PointerRangeView(rhs.*ptrMember, rhs.*sizeMember);
        }
    }

    template<typename A>
    bool operator()(const A& lhs, const A& rhs) const {
        if (compare(lhs, rhs, M1)) return true;
        if (compare(rhs, lhs, M1)) return false;
        return MemberComparator<M...>()(lhs, rhs);
    }
};

template<>
struct MemberComparator<> {
    template<typename A>
    bool operator()(const A& lhs, const A& rhs) const {
        return false;
    }
};

template<typename T>
struct DescriptorOrder {
    bool operator()(const T &lhs, const T &rhs) const {
        return lhs < rhs;
    }
};

template<typename T>
struct DescriptorOrder<std::vector<T>> {

    bool operator()(const std::vector<T> &lhs, const std::vector<T> &rhs) const {
        if(lhs.size() < rhs.size()) return true;
        if(rhs.size() < lhs.size()) return false;
        for(std::size_t i = 0; i < lhs.size(); ++i) {
            DescriptorOrder<T> compare;
            if(compare(lhs[i], rhs[i])) return true;
            if(compare(rhs[i], lhs[i])) return false;
        }
        return false;
    }
};

template<typename T>
struct EnumDescriptorOrder {
    bool operator()(const T& lhs, const T& rhs) const {
        return static_cast<std::underlying_type_t<T>>(lhs) < static_cast<std::underlying_type_t<T>>(rhs);
    }
};

}

#include "VkOrder.h"
