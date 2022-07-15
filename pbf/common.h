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
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <crampl/crampl.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <vulkan/vulkan.hpp>

namespace pbf {
class ContextInterface;
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

namespace crampl_candidate {

template<typename T>
class ClosureContainer;

template<typename Ret, typename... Args>
class ClosureContainer<Ret(Args...)> {
public:
    using ArgumentTypes = std::tuple<Args...>;
    using ReturnType = Ret;
    virtual ~ClosureContainer() = default;
    virtual Ret operator()(Args...) = 0;
};

template<typename CallableType, typename T>
class TypedClosureContainer;

template<typename R, typename T, typename... Args>
class TypedClosureContainer<R(Args...), T>: public ClosureContainer<R(Args...)> {
public:
    using type = T;
    TypedClosureContainer(T&& t): callable(std::move(t)) {}
    virtual ~TypedClosureContainer() = default;
    R operator()(Args&&... args) override {
        if constexpr(std::is_same_v<R, void>) {
            callable(std::forward<Args>(args)...);
        } else {
            return callable(std::forward<Args>(args)...);
        }
    }
private:
    T callable;
};

}

class ClosureContainer {
public:
    virtual ~ClosureContainer() = default;
    virtual void operator()(vk::CommandBuffer&) = 0;
};
template<typename T>
class TypedClosureContainer: public ClosureContainer {
public:
    using type = T;
    TypedClosureContainer(T&& t): callable(std::move(t)) {}
    virtual ~TypedClosureContainer() = default;
    void operator()(vk::CommandBuffer& cb) override {
        callable(cb);
    }
private:
    T callable;
};

}

#ifndef NDEBUG
#define PBF_DESC_DEBUG_NAME(name) .debugName = name
#else
#define PBF_DESC_DEBUG_NAME(name)
#endif