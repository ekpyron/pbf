#pragma once

#include <vulkan/vulkan.hpp>

namespace pbf {

template<typename T>
constexpr vk::ObjectType VulkanObjectType = T::objectType;

template<typename T>
concept KnownVulkanObject = requires(T) {T::objectType;}
                            && std::is_same_v<std::decay_t<decltype(T::objectType)>, vk::ObjectType>;

}
