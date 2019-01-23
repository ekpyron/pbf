#pragma once

#include <vulkan/vulkan.hpp>
#include <pbf/common.h>
#include <pbf/glfw.h>
#include <pbf/descriptors/RenderPass.h>
#include <pbf/Cache.h>
#include <pbf/VulkanObjectType.h>

namespace pbf {

class Context {
public:
    Context();
    virtual ~Context();

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
    Cache &cache() { return _cache; }
    const Renderer &renderer() const { return *_renderer; }

#ifndef NDEBUG
    template<typename T>
    void setObjectName(const T &obj, const std::string &name) {
        setGenericObjectName(VulkanObjectType<T>, reinterpret_cast<uint64_t>(static_cast<void*>(obj)), name);
    }
    void setGenericObjectName(vk::ObjectType type, uint64_t obj, const std::string &name) const;
#define PBF_DEBUG_SET_OBJECT_NAME(context, object, name) context->setObjectName(object, name)
#else
#define PBF_DEBUG_SET_OBJECT_NAME(context, object, name) do { } while(0)
#endif

protected:
	virtual int ratePhysicalDevice(const vk::PhysicalDevice &physicalDevice) const {
    	int rating = 0;
		if (physicalDevice.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			rating += 1000;
		return rating;
    }
private:
    auto getPhysicalDevice() {
        auto devices = _instance->enumeratePhysicalDevices();
        std::map<int, const vk::PhysicalDevice*> ratedDevices;
        for(const auto &physicalDevice : devices) {
            ratedDevices.emplace(ratePhysicalDevice(physicalDevice), &physicalDevice);
        }
        for (auto it = ratedDevices.rbegin(); it != ratedDevices.rend(); ++it) {
            const auto &physicalDevice = *it->second;
            {
                const auto &properties = physicalDevice.enumerateDeviceExtensionProperties();
                auto propertyIt = std::find_if(properties.begin(), properties.end(), [](const auto &prop) {
                    return prop.extensionName == std::string(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
                });
                if (propertyIt == properties.end()) continue;
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
        throw std::runtime_error("No suitable discrete GPU found.");
    }

    glfw::GLFW _glfw;
    std::unique_ptr<glfw::Window> _window;

    vk::UniqueInstance _instance;
#ifndef NDEBUG
    VkBool32 debugUtilMessengerCallback(vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity,
            vk::DebugUtilsMessageTypeFlagsEXT messageType,
            const vk::DebugUtilsMessengerCallbackDataEXT &callbackData) const;

    std::unique_ptr<vk::DispatchLoaderDynamic> dldi;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> _debugUtilsMessenger;
#endif
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
    vk::UniqueSurfaceKHR _surface;
    vk::Queue _graphicsQueue, _presentQueue;

    vk::SurfaceFormatKHR _surfaceFormat {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
    vk::PresentModeKHR _presentMode {vk::PresentModeKHR::eFifo};

    vk::UniqueCommandPool _commandPool;

    Cache _cache { this, 100 };

    std::unique_ptr<Renderer> _renderer;
};

}
