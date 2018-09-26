#pragma once

#include <vulkan/vulkan.hpp>
#include "common.h"
#include "glfw.h"

namespace pbf {

class Renderer;

class Context {
public:
    Context();
    ~Context();

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    void run();

    const vk::Device &device() const { return *_device; }
    const vk::PhysicalDevice &physicalDevice() const { return _physicalDevice; }
    const vk::SurfaceKHR &surface() const { return *_surface; }
    const glfw::Window &window() const { return *_window; }
    const vk::SurfaceFormatKHR &surfaceFormat() const { return _surfaceFormat; }
    const vk::PresentModeKHR &presentMode() const { return _presentMode; }
    const auto &families() const { return _families; }
    const vk::Queue &graphicsQueue() const { return _graphicsQueue; }
    const vk::Queue &presentQueue() const { return _presentQueue; }
    const vk::CommandPool &commandPool() const { return *_commandPool; }

private:

    auto getPhysicalDevice() {
        for(auto &physicalDevice : _instance->enumeratePhysicalDevices()) {
            if(physicalDevice.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                {
                    const auto &properties = physicalDevice.enumerateDeviceExtensionProperties();
                    auto it = std::find_if(properties.begin(), properties.end(), [](const auto &prop) {
                        return prop.extensionName == std::string(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
                    });
                    if (it == properties.end()) continue;
                }
                int graphicsFamily = -1;
                int presentFamily = -1;
                {
                    const auto& queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
                    for(std::size_t i = 0; i < queueFamilyProperties.size(); ++i) {
                        const auto& qf = queueFamilyProperties[i];
                        if(qf.queueFlags & vk::QueueFlagBits::eGraphics) {
                            graphicsFamily = static_cast<int>(i);
                        }
                        if(physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *_surface)) {
                            presentFamily = static_cast<int>(i);
                        }
                        if(graphicsFamily >= 0 && presentFamily >= 0) break;
                    }

                    if(!(graphicsFamily >= 0 && presentFamily >= 0)) continue;
                }
                {
                    if(physicalDevice.getSurfaceFormatsKHR(*_surface).empty()) continue;
                    if(physicalDevice.getSurfacePresentModesKHR(*_surface).empty()) continue;
                }
                return std::make_tuple(physicalDevice,
                        static_cast<std::uint32_t>(graphicsFamily), static_cast<std::uint32_t>(presentFamily));
            }
        }
        throw std::runtime_error("No suitable discrete GPU found.");
    }

    VkBool32
    debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, std::uint64_t object,
                        std::size_t location, std::int32_t messageCode, const char *pLayerPrefix, const char *pMessage);

    glfw::GLFW _glfw;
    std::unique_ptr<glfw::Window> _window;

    vk::UniqueInstance _instance{nullptr};
#ifndef NDEBUG
    std::unique_ptr<vk::DispatchLoaderDynamic> dldi;
    vk::UniqueHandle<vk::DebugReportCallbackEXT, vk::DispatchLoaderDynamic> _debugReportCallback;
#endif
    vk::UniqueSurfaceKHR _surface;
    vk::PhysicalDevice _physicalDevice;

    union {
        struct {
            std::uint32_t graphics;
            std::uint32_t present;
        };
        std::array<std::uint32_t, 2> arr;

        bool same() const {
            return graphics == present;
        }
    } _families;

    vk::UniqueDevice _device;
    vk::Queue _graphicsQueue, _presentQueue;

    vk::SurfaceFormatKHR _surfaceFormat {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
    vk::PresentModeKHR _presentMode {vk::PresentModeKHR::eFifo};

    vk::UniqueCommandPool _commandPool;

    std::unique_ptr<Renderer> _renderer;
};

}
