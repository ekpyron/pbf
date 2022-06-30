#pragma once

#include <vulkan/vulkan.hpp>

namespace pbf {

namespace detail {

template<typename T>
struct VulkanObjectType;

template<> struct VulkanObjectType<vk::Instance> : std::integral_constant<vk::ObjectType, vk::ObjectType::eInstance> {};
template<> struct VulkanObjectType<vk::PhysicalDevice> : std::integral_constant<vk::ObjectType, vk::ObjectType::ePhysicalDevice> {};
template<> struct VulkanObjectType<vk::Device> : std::integral_constant<vk::ObjectType, vk::ObjectType::eDevice> {};
template<> struct VulkanObjectType<vk::Queue> : std::integral_constant<vk::ObjectType, vk::ObjectType::eQueue> {};
template<> struct VulkanObjectType<vk::Semaphore> : std::integral_constant<vk::ObjectType, vk::ObjectType::eSemaphore> {};
template<> struct VulkanObjectType<vk::CommandBuffer> : std::integral_constant<vk::ObjectType, vk::ObjectType::eCommandBuffer> {};
template<> struct VulkanObjectType<vk::Fence> : std::integral_constant<vk::ObjectType, vk::ObjectType::eFence> {};
template<> struct VulkanObjectType<vk::DeviceMemory> : std::integral_constant<vk::ObjectType, vk::ObjectType::eDeviceMemory> {};
template<> struct VulkanObjectType<vk::Buffer> : std::integral_constant<vk::ObjectType, vk::ObjectType::eBuffer> {};
template<> struct VulkanObjectType<vk::Image> : std::integral_constant<vk::ObjectType, vk::ObjectType::eImage> {};
template<> struct VulkanObjectType<vk::Event> : std::integral_constant<vk::ObjectType, vk::ObjectType::eEvent> {};
template<> struct VulkanObjectType<vk::QueryPool> : std::integral_constant<vk::ObjectType, vk::ObjectType::eQueryPool> {};
template<> struct VulkanObjectType<vk::BufferView> : std::integral_constant<vk::ObjectType, vk::ObjectType::eBufferView> {};
template<> struct VulkanObjectType<vk::ImageView> : std::integral_constant<vk::ObjectType, vk::ObjectType::eImageView> {};
template<> struct VulkanObjectType<vk::ShaderModule> : std::integral_constant<vk::ObjectType, vk::ObjectType::eShaderModule> {};
template<> struct VulkanObjectType<vk::PipelineCache> : std::integral_constant<vk::ObjectType, vk::ObjectType::ePipelineCache> {};
template<> struct VulkanObjectType<vk::PipelineLayout> : std::integral_constant<vk::ObjectType, vk::ObjectType::ePipelineLayout> {};
template<> struct VulkanObjectType<vk::RenderPass> : std::integral_constant<vk::ObjectType, vk::ObjectType::eRenderPass> {};
template<> struct VulkanObjectType<vk::Pipeline> : std::integral_constant<vk::ObjectType, vk::ObjectType::ePipeline> {};
template<> struct VulkanObjectType<vk::DescriptorSetLayout> : std::integral_constant<vk::ObjectType, vk::ObjectType::eDescriptorSetLayout> {};
template<> struct VulkanObjectType<vk::Sampler> : std::integral_constant<vk::ObjectType, vk::ObjectType::eSampler> {};
template<> struct VulkanObjectType<vk::DescriptorPool> : std::integral_constant<vk::ObjectType, vk::ObjectType::eDescriptorPool> {};
template<> struct VulkanObjectType<vk::DescriptorSet> : std::integral_constant<vk::ObjectType, vk::ObjectType::eDescriptorSet> {};
template<> struct VulkanObjectType<vk::Framebuffer> : std::integral_constant<vk::ObjectType, vk::ObjectType::eFramebuffer> {};
template<> struct VulkanObjectType<vk::CommandPool> : std::integral_constant<vk::ObjectType, vk::ObjectType::eCommandPool> {};
template<> struct VulkanObjectType<vk::SamplerYcbcrConversion> : std::integral_constant<vk::ObjectType, vk::ObjectType::eSamplerYcbcrConversion> {};
template<> struct VulkanObjectType<vk::DescriptorUpdateTemplate> : std::integral_constant<vk::ObjectType, vk::ObjectType::eDescriptorUpdateTemplate> {};
template<> struct VulkanObjectType<vk::SurfaceKHR> : std::integral_constant<vk::ObjectType, vk::ObjectType::eSurfaceKHR> {};
template<> struct VulkanObjectType<vk::SwapchainKHR> : std::integral_constant<vk::ObjectType, vk::ObjectType::eSwapchainKHR> {};
template<> struct VulkanObjectType<vk::DisplayKHR> : std::integral_constant<vk::ObjectType, vk::ObjectType::eDisplayKHR> {};
template<> struct VulkanObjectType<vk::DisplayModeKHR> : std::integral_constant<vk::ObjectType, vk::ObjectType::eDisplayModeKHR> {};
template<> struct VulkanObjectType<vk::DebugReportCallbackEXT> : std::integral_constant<vk::ObjectType, vk::ObjectType::eDebugReportCallbackEXT> {};
/*template<> struct VulkanObjectType<vk::ObjectTableNVX> : std::integral_constant<vk::ObjectType, vk::ObjectType::eObjectTableNVX> {};
template<> struct VulkanObjectType<vk::IndirectCommandsLayoutNVX> : std::integral_constant<vk::ObjectType, vk::ObjectType::eIndirectCommandsLayoutNVX> {};*/
template<> struct VulkanObjectType<vk::DebugUtilsMessengerEXT> : std::integral_constant<vk::ObjectType, vk::ObjectType::eDebugUtilsMessengerEXT> {};
template<> struct VulkanObjectType<vk::ValidationCacheEXT> : std::integral_constant<vk::ObjectType, vk::ObjectType::eValidationCacheEXT> {};
template<> struct VulkanObjectType<vk::AccelerationStructureNV> : std::integral_constant<vk::ObjectType, vk::ObjectType::eAccelerationStructureNV> {};

template<typename T, typename = void>
struct IsKnownVulkanObject : std::false_type {};
template<typename T>
struct IsKnownVulkanObject<T, std::void_t<decltype(VulkanObjectType<T>::value)>> : std::true_type {};

}

template<typename T>
constexpr vk::ObjectType VulkanObjectType = detail::VulkanObjectType<T>::value;

template<typename T>
constexpr bool IsKnownVulkanObject = detail::IsKnownVulkanObject<T>::value;

}
