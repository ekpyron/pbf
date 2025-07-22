/**
 *
 *
 * @file VulkanContext.cpp
 * @brief 
 * @author clonker
 * @date 9/26/18
 */

#include <regex>
#include <pbf/descriptors/RenderPass.h>
#include <pbf/descriptors/DescriptorSetLayout.h>
#include <contrib/crampl/crampl/ContainerContainer.h>
#include "VulkanContext.h"

#include <iostream>

#include "Renderer.h"
#include "SurfaceReconstruction.h"
#include "Scene.h"
#include "Selection.h"
#include "GUI.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_projection.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace pbf {


VulkanContext::VulkanContext() {
    if (!_glfw.vulkanSupported()) {
        throw std::runtime_error("Vulkan not supported");
    }
    _glfw.windowHint(GLFW_CLIENT_API, GLFW_NO_API);
	_glfw.windowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
	/*int count = 0;
	auto monitors = glfwGetMonitors(&count);
	//auto* monitor = glfwGetPrimaryMonitor();
	auto* monitor = monitors[1];
	auto* mode = glfwGetVideoMode(monitor);
	_glfw.windowHint(GLFW_RED_BITS, mode->redBits);
	_glfw.windowHint(GLFW_GREEN_BITS, mode->greenBits);
	_glfw.windowHint(GLFW_BLUE_BITS, mode->blueBits);
	_glfw.windowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    _window = std::make_unique<glfw::Window>(mode->width, mode->height, "PBF", monitor, nullptr);*/
	_window = std::make_unique<glfw::Window>(768*2, 768, "PBF", nullptr, nullptr);

    {
        std::vector<const char *> layers;
        auto extensions = _glfw.getRequiredInstanceExtensions();
#ifndef NDEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        layers.push_back("VK_LAYER_KHRONOS_validation");
    	//layers.push_back("VK_LAYER_DEV_self_validation");
#endif
        vk::ApplicationInfo appInfo{
                .pApplicationName = "PBF",
                .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
                .pEngineName = "PBF",
                .engineVersion = VK_MAKE_VERSION(0, 0, 0),
                .apiVersion = VK_API_VERSION_1_3
        };
#ifndef NDEBUG
        bool gpuAssistedValidation = false;
		std::array enables = {
			vk::ValidationFeatureEnableEXT::eGpuAssisted
		};
		vk::ValidationFeaturesEXT validationFeatures{
			.enabledValidationFeatureCount = enables.size(),
			.pEnabledValidationFeatures = enables.data()
		};
        {
        	uint32_t count;
        	vkEnumerateInstanceLayerProperties(&count, nullptr);
        	std::vector<VkLayerProperties> availableLayers(count);
        	vkEnumerateInstanceLayerProperties(&count, availableLayers.data());
        	spdlog::get("console")->info("Found {} layers", count);
        	for (const auto& layer : availableLayers)
        	{
        		spdlog::get("console")->info("Has layer {}", layer.layerName);
        	}

        }
#endif
        _instance = vk::createInstanceUnique(vk::InstanceCreateInfo{
#ifndef NDEBUG
			.pNext = gpuAssistedValidation ? &validationFeatures : nullptr,
#endif
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(layers.size()),
			.ppEnabledLayerNames = layers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data()
		 }, nullptr);
    }
#ifndef NDEBUG
    dldi = std::make_unique<vk::detail::DispatchLoaderDynamic>(*_instance, ::vkGetInstanceProcAddr);
    _debugUtilsMessenger = _instance->createDebugUtilsMessengerEXTUnique(
            vk::DebugUtilsMessengerCreateInfoEXT {
				.messageSeverity = ~vk::DebugUtilsMessageSeverityFlagBitsEXT(),
				.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral|vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance|vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
				.pfnUserCallback = [](vk::DebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
                       vk::DebugUtilsMessageTypeFlagsEXT                  messageType,
                       const vk::DebugUtilsMessengerCallbackDataEXT*      pCallbackData,
                       void*                                            pUserData) -> VkBool32 {
                        return static_cast<VulkanContext *>(pUserData)->debugUtilMessengerCallback(
                                messageSeverity,
                                messageType,
                                *pCallbackData
                            );
                        },
				.pUserData = this
            }, nullptr, *dldi);
#endif

    _surface = _window->createSurface(*_instance);

    std::tie(_physicalDevice, _families.graphics, _families.present) = getPhysicalDevice();

    {
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.f;
        if (_families.same()) {
            queueCreateInfos = {vk::DeviceQueueCreateInfo{
				.queueFamilyIndex  = _families.graphics,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority
			}};
        } else {
            queueCreateInfos = {vk::DeviceQueueCreateInfo{
				.queueFamilyIndex = _families.graphics,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority
			},
			vk::DeviceQueueCreateInfo{
				.queueFamilyIndex = _families.present,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority
			}};
        }
        vk::PhysicalDeviceFeatures features{};
        features.setMultiDrawIndirect(static_cast<vk::Bool32>(true));
        vk::PhysicalDeviceMaintenance4Features maintenance4Features{
            .maintenance4 = vk::True
        };
        auto extensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        _device = _physicalDevice.createDeviceUnique(vk::DeviceCreateInfo{
            .pNext = &maintenance4Features,
			.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
			.pQueueCreateInfos = queueCreateInfos.data(),
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = 1,
			.ppEnabledExtensionNames = &extensionName,
			.pEnabledFeatures = &features
		});

        _graphicsQueue = _device->getQueue(_families.graphics, 0);
        if (_families.graphics != _families.present) {
            PBF_DEBUG_SET_OBJECT_NAME(*this, _graphicsQueue, "Graphics Queue");
            _presentQueue = _device->getQueue(_families.present, 0);
            PBF_DEBUG_SET_OBJECT_NAME(*this, _presentQueue, "Present Queue");

        } else {
            _presentQueue = _graphicsQueue;
            PBF_DEBUG_SET_OBJECT_NAME(*this, _presentQueue, "Graphics & Present Queue");
        }
    }

    {
        const auto &surfaceFormats = _physicalDevice.getSurfaceFormatsKHR(*_surface);
        if(surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined) {
            // fall through
        } else {
            auto it = std::find(surfaceFormats.begin(), surfaceFormats.end(), _surfaceFormat);
            if(it == surfaceFormats.end()) {
                _surfaceFormat = surfaceFormats[0];
            }
        }
    }

    {
        const auto &modes = _physicalDevice.getSurfacePresentModesKHR(*_surface);
		for (auto [desiredPresentMode, label] : {
            std::make_tuple(vk::PresentModeKHR::eMailbox, "Mailbox"),
            std::make_tuple(vk::PresentModeKHR::eFifoRelaxed, "FifoRelaxed"),
            std::make_tuple(vk::PresentModeKHR::eFifo, "Fifo")
        })
		{
			auto it = std::find(std::begin(modes), std::end(modes), desiredPresentMode);
			if (it != std::end(modes)) {
				_presentMode = desiredPresentMode;
                spdlog::get("console")->info("[Context] Selected present mode {}", label);
				break;
			}
		}
    }

	_descriptorPool = _device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
		.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, // TODO: reconsider
		.maxSets = maxGlobalDescriptorSets,
		.poolSizeCount = static_cast<std::uint32_t>(globalDescriptorPoolSizes().size()),
		.pPoolSizes = globalDescriptorPoolSizes().data()
	});

    _memoryManager = std::make_unique<MemoryManager>(*this);

    _commandPool = _device->createCommandPoolUnique(vk::CommandPoolCreateInfo{.queueFamilyIndex = _families.graphics});
    _commandPoolTransient = _device->createCommandPoolUnique(vk::CommandPoolCreateInfo{
		.flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = _families.graphics
	});
    PBF_DEBUG_SET_OBJECT_NAME(*this, *_commandPool, "Main Command Pool");
}

VulkanContext::~VulkanContext()
{
}

void VulkanContext::pollEvents()
{
	_glfw.pollEvents();
}


#ifndef NDEBUG
VkBool32
VulkanContext::debugUtilMessengerCallback(vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT &callbackData) const {
	auto logger = spdlog::get("vulkan");
    std::string messageTypeString = "Unknown";
    std::map<std::string, std::string> nameMap;
    std::string message = callbackData.pMessage ? callbackData.pMessage : "[null]";
    for (uint32_t i = 0; i < callbackData.objectCount; i++)
    {
        if (callbackData.pObjects[i].pObjectName)
            message = std::regex_replace(message, std::regex(fmt::format("obj {:#x}", callbackData.pObjects[i].objectHandle)),
                                                    fmt::format("[{} ({:#x})]", callbackData.pObjects[i].pObjectName,
                                                            callbackData.pObjects[i].objectHandle));
    }

    if (messageType & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
        messageTypeString = "General";
    else if (messageType & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
        messageTypeString = "Validation";
    else if (messageType & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        messageTypeString = "Performance";
    if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        logger->error("[{}] {}", messageTypeString, message);
    	//assert(false); // Be easy on the GPU.
    } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        logger->warn("[{}] {}", messageTypeString, message);
    } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
        logger->info("[{}] {}", messageTypeString, message);
    } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose) {
        logger->debug("[{}] {}", messageTypeString, message);
    }
    return VK_FALSE;
}

void VulkanContext::setGenericObjectName(vk::ObjectType type, uint64_t obj, const std::string &name) const {
    //spdlog::get("vulkan")->info("Assign name [{}] = object {:#x}", name, obj);
    _device->setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		.objectType = type,
		.objectHandle = obj,
		.pObjectName = name.c_str()
    }, *dldi);
}
#endif

vk::Format VulkanContext::depthFormat() const
{
	// TODO
	return vk::Format::eD32Sfloat;
}

}
