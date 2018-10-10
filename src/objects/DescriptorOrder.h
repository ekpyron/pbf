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

namespace pbf {


template<typename T>
struct DescriptorOrder;

template<typename T>
struct DescriptorOrder {
    bool operator()(const T &lhs, const T &rhs) const {
        return lhs < rhs;
    }
};

template<typename T>
struct DescriptorOrder<std::optional<T>> {

    bool operator()(const std::optional<T> &lhs, const std::optional<T> &rhs) const {
        if(!rhs) return false;
        if(!lhs) return true;
        return DescriptorOrder<T>()(lhs.value(), rhs.value());
    }
};

template<typename T>
struct DescriptorOrder<std::vector<T>> {

    bool operator()(const std::vector<T> &lhs, const std::vector<T> &rhs) const {
        DescriptorOrder<T> compare;
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), compare);
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
