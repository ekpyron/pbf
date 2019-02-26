/**
 *
 *
 * @file common.h
 * @brief 
 * @author clonker
 * @date 9/26/18
 */
#pragma once

#include <ostream>
#include <vector>

#include <glm/glm.hpp>

#include <crampl/crampl.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <vulkan/vulkan.hpp>

namespace pbf {
class Context;
class Renderer;
class MemoryManager;
class Scene;

template<template<typename...> typename Container, typename T, typename... Tail>
class SizeAdjustedContainer: public Container<T, Tail...> {
public:
    SizeAdjustedContainer(std::initializer_list<T>&& l): Container<T, Tail...>(std::forward<std::initializer_list<T>>(l)) {}
    template<typename... Args>
    SizeAdjustedContainer(Args&&... args): Container<T, Tail...>(std::forward<Args>(args)...) {}
    std::uint32_t size() const {
        return static_cast<std::uint32_t>(Container<T, Tail...>::size());
    }

};

template<typename... T>
using vector32 = SizeAdjustedContainer<std::vector,T...>;

namespace descriptors {
class GraphicsPipeline;
class RenderPass;
class DescriptorSetLayout;
}

enum class MemoryType {
    STATIC, TRANSIENT, DYNAMIC
};

inline std::ostream& operator<<(std::ostream &os, const MemoryType& memoryType) {
    switch (memoryType) {
        case MemoryType::STATIC: { os << "STATIC"; break; }
        case MemoryType::TRANSIENT:{ os << "TRANSIENT"; break; }
        case MemoryType::DYNAMIC:{ os << "DYNAMIC"; break; }
    }
    return os;
}

}

#ifndef NDEBUG
#define PBF_DESC_DEBUG_NAME(name) .debugName = name
#else
#define PBF_DESC_DEBUG_NAME(name)
#endif