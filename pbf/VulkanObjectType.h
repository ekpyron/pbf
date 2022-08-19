#pragma once

#include <vulkan/vulkan.hpp>

namespace pbf {

template<typename T>
constexpr vk::ObjectType VulkanObjectType = T::objectType;

template<typename T>
concept KnownVulkanObject = requires(T) { {T::objectType} -> std::convertible_to<vk::ObjectType>;};

}
