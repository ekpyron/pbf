#pragma once

#include <vulkan/vulkan.hpp>
#include <pbf/common.h>
#include <pbf/glfw.h>
#include <pbf/Cache.h>
#include <pbf/VulkanObjectType.h>
#include "MemoryManager.h"
#include "Camera.h"
#include "ContextInterface.h"

namespace pbf {

class GUI;

class VulkanContext : public ContextInterface {
public:
    VulkanContext();
    virtual ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

	vk::Instance instance() const { return *_instance; }
    const vk::Device &device() const override { return *_device; }
    const vk::PhysicalDevice &physicalDevice() const override { return _physicalDevice; }
    const vk::SurfaceKHR &surface() const { return *_surface; }
    const glfw::Window &window() const { return *_window; }
	void pollEvents();
    const vk::SurfaceFormatKHR &surfaceFormat() const { return _surfaceFormat; }
    const vk::PresentModeKHR &presentMode() const { return _presentMode; }
    const auto &families() const { return _families; }
    const vk::Queue &graphicsQueue() const { return _graphicsQueue; }
    const vk::Queue &presentQueue() const { return _presentQueue; }
    const vk::CommandPool &commandPool(bool transient) const { return *(transient ? _commandPoolTransient : _commandPool); }
    Cache &cache() override { return _cache; }
    MemoryManager &memoryManager() override { return *_memoryManager; }
	vk::Format depthFormat() const;

	uint32_t graphicsQueueFamily() const {
		return _families.graphics;
	}

	vk::DescriptorPool descriptorPool() const override { return *_descriptorPool; };

#ifndef NDEBUG
    template<typename T>
    void setObjectName(const T &obj, const std::string &name) {
        setGenericObjectName(VulkanObjectType<T>, reinterpret_cast<uint64_t>(static_cast<void*>(obj)), name);
    }
    void setGenericObjectName(vk::ObjectType type, uint64_t obj, const std::string &name) const override;
#define PBF_DEBUG_SET_OBJECT_NAME(context, object, name) (context).setObjectName(object, name)
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
    auto getPhysicalDevice(auto const& desiredExtensions) {
        auto devices = _instance->enumeratePhysicalDevices();
        std::map<int, const vk::PhysicalDevice*> ratedDevices;
        for(const auto &physicalDevice : devices) {
            ratedDevices.emplace(ratePhysicalDevice(physicalDevice), &physicalDevice);
        }
        for (auto it = ratedDevices.rbegin(); it != ratedDevices.rend(); ++it) {
            const auto &physicalDevice = *it->second;
            {
                const auto &properties = physicalDevice.enumerateDeviceExtensionProperties();
            	bool allDesiredPresent = true;
            	for (auto const& ext: desiredExtensions)
            	{
            		auto propertyIt = std::find_if(properties.begin(), properties.end(), [&](const auto &prop) {
						return std::string(prop.extensionName.data()) == ext;
					});
            		if (propertyIt == properties.end()) {
            			spdlog::get("console")->debug("Rejecting device due to missing {}", ext);
            			allDesiredPresent = false;
            			break;
            		}
            	}
            	if (!allDesiredPresent)
            		continue;
            }
            int graphicsFamily = -1;
            int presentFamily = -1;
            {
                const auto& queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
                for(std::size_t i = 0; i < queueFamilyProperties.size(); ++i) {
                    const auto& qf = queueFamilyProperties[i];
                    if((qf.queueFlags & (vk::QueueFlagBits::eGraphics|vk::QueueFlagBits::eCompute)) == (vk::QueueFlagBits::eGraphics|vk::QueueFlagBits::eCompute)) {
                        graphicsFamily = static_cast<int>(i);
                    }
                    if(physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *_surface)) {
                        presentFamily = static_cast<int>(i);
                    }
                    if(graphicsFamily >= 0 && presentFamily >= 0) break;
                }

                if(!(graphicsFamily >= 0 && presentFamily >= 0)) {
					spdlog::get("console")->debug("Rejecting device due to missing graphics and present family");
					continue;
				}
            }
            {
                if(physicalDevice.getSurfaceFormatsKHR(*_surface).empty()) {
					spdlog::get("console")->debug("Rejecting device due to missing surface formats");
					continue;
				}
                if(physicalDevice.getSurfacePresentModesKHR(*_surface).empty()) {
					spdlog::get("console")->debug("Rejecting device due to missing present modes");
					continue;
				}
            }
            return std::make_tuple(physicalDevice,
                    static_cast<std::uint32_t>(graphicsFamily), static_cast<std::uint32_t>(presentFamily));
        }
        throw std::runtime_error("No suitable discrete GPU found.");
    }

    glfw::GLFW _glfw;
    std::unique_ptr<glfw::Window> _window;

    vk::detail::DispatchLoaderStatic dls;
    vk::UniqueInstance _instance;
#ifndef NDEBUG
    std::unique_ptr<vk::detail::DispatchLoaderDynamic> dldi;
    VkBool32 debugUtilMessengerCallback(vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity,
            vk::DebugUtilsMessageTypeFlagsEXT messageType,
            const vk::DebugUtilsMessengerCallbackDataEXT &callbackData) const;

    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic> _debugUtilsMessenger;
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

	static constexpr std::uint32_t maxGlobalDescriptorSets = 1024;
    static const auto &globalDescriptorPoolSizes() {
        static std::array<vk::DescriptorPoolSize, 4> sizes {{{
			vk::DescriptorType::eUniformBuffer, 512
		}, {
			vk::DescriptorType::eStorageBuffer, 512
		}, {
			vk::DescriptorType::eCombinedImageSampler, 512
		}, {
			vk::DescriptorType::eStorageImage, 512
		}}};
        return sizes;
    }

    vk::UniqueDescriptorPool _descriptorPool;

    vk::UniqueCommandPool _commandPool;
    vk::UniqueCommandPool _commandPoolTransient;

    std::unique_ptr<MemoryManager> _memoryManager;
    Cache _cache { *this, 100 };
};

struct InitContext {
	InitContext(VulkanContext& context): context(context) {
		initCommandBuffer = std::move(context.device().allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
			.commandPool = context.commandPool(true),
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = 1U
		}).front());
	}

	template<typename T, typename... Args>
	T& createInitData(Args&&... args) {
		auto uniqueFrameData = std::make_unique<FrameData<T>>(std::forward<Args>(args)...);
		T& result = uniqueFrameData->data;
		initData.emplace_back(move(uniqueFrameData));
		return result;
	}

	VulkanContext& context;
	vk::UniqueCommandBuffer initCommandBuffer;
	std::vector<std::unique_ptr<FrameDataBase>> initData{};

};

template<typename T>
struct InitWrapper
{
	InitContext& context;
	T& t;
};

}
