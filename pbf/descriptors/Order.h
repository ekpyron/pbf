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

namespace pbf::descriptors {

template<typename T, typename U>
using enable_if_same_t = std::enable_if_t<std::is_same_v<std::decay_t<T>, std::decay_t<U>>>;

template<typename T, typename = void>
struct Order {
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
        Order<std::decay_t<T>> compare;
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), compare);
    }
};

template<typename T>
struct Order<vector32<T>>: Order<std::vector<T>> {};

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

}

#include "VkOrder.h"
