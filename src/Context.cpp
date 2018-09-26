/**
 *
 *
 * @file VulkanContext.cpp
 * @brief 
 * @author clonker
 * @date 9/26/18
 */

#include "Context.h"
#include "Swapchain.h"

namespace pbf {


Context::Context() {
    if (!_glfw.vulkanSupported()) {
        throw std::runtime_error("Vulkan not supported");
    }
    _glfw.windowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = std::make_unique<glfw::Window>(700, 700, "PBF", nullptr, nullptr);

    {
        std::vector<const char *> layers;
        auto extensions = _glfw.getRequiredInstanceExtensions();
#ifndef NDEBUG
        extensions.push_back("VK_EXT_debug_report");
        //layers.push_back("VK_LAYER_LUNARG_standard_validation"); //leads to crash
#endif
        vk::ApplicationInfo appInfo{
                .pApplicationName = "PBF",
                .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
                .pEngineName = "PBF",
                .engineVersion = VK_MAKE_VERSION(0, 0, 0),
                .apiVersion = VK_API_VERSION_1_1
        };
        _instance = vk::createInstanceUnique({
                                                     {}, &appInfo, static_cast<uint32_t>(layers.size()), layers.data(),
                                                     static_cast<uint32_t>(extensions.size()), extensions.data()
                                             });
    }
#ifndef NDEBUG
    auto debugFlags = vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::eError |
                      vk::DebugReportFlagBitsEXT::eInformation | vk::DebugReportFlagBitsEXT::ePerformanceWarning |
                      vk::DebugReportFlagBitsEXT::eWarning;
    auto callback = [](VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, std::uint64_t object,
                       std::size_t location, std::int32_t messageCode, const char *pLayerPrefix, const char *pMessage,
                       void *userData) -> VkBool32 {
        return static_cast<Context *>(userData)->debugReportCallback(flags, objectType, object, location, messageCode,
                                                                     pLayerPrefix, pMessage);
    };
    dldi = std::make_unique<vk::DispatchLoaderDynamic>(*_instance);
    _debugReportCallback = _instance->createDebugReportCallbackEXTUnique(vk::DebugReportCallbackCreateInfoEXT{
            debugFlags, callback, this
    }, nullptr, *dldi);
#endif

    _surface = _window->createSurface(*_instance);

    std::tie(_physicalDevice, _families.graphics, _families.present) = getPhysicalDevice();

    {
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.f;
        if (_families.same()) {
            queueCreateInfos = {{{}, uint32_t(_families.graphics), 1, &queuePriority}};
        } else {
            queueCreateInfos = {{{}, uint32_t(_families.graphics), 1, &queuePriority},
                                {{}, uint32_t(_families.present),  1, &queuePriority}};
        }
        vk::PhysicalDeviceFeatures features{};
        std::vector<const char *> layers;
#ifndef NDEBUG
        layers.push_back("VK_LAYER_LUNARG_standard_validation");
#endif
        auto extensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        _device = _physicalDevice.createDeviceUnique(
                {{}, static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(),
                 static_cast<uint32_t>(layers.size()), layers.data(), 1, &extensionName, &features});

        _graphicsQueue = _device->getQueue(static_cast<uint32_t>(_families.graphics), 0);
        _presentQueue = _device->getQueue(static_cast<uint32_t>(_families.present), 0);
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
        auto it = std::find(std::begin(modes), std::end(modes), vk::PresentModeKHR::eMailbox);
        if(it != std::end(modes)) _presentMode = *it;
    }

    _swapchain = std::make_unique<Swapchain>(this);
}

Context::~Context() = default;

VkBool32
Context::debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, std::uint64_t object,
                             std::size_t location, std::int32_t messageCode, const char *pLayerPrefix,
                             const char *pMessage) {
    auto logger = spdlog::get("console");
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        logger->critical("Vulkan ({}): \"{}\", Object: {:#x}", pLayerPrefix, pMessage, object);
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        logger->warn("Vulkan ({}): \"{}\", Object: {:#x}", pLayerPrefix, pMessage, object);
    } else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        logger->warn("Vulkan performance ({}): \"{}\", Object: {:#x}", pLayerPrefix, pMessage, object);
    } else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        logger->debug("Vulkan ({}): \"{}\", Object: {:#x}", pLayerPrefix, pMessage, object);
    } else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        logger->info("Vulkan ({}): \"{}\", Object: {:#x}", pLayerPrefix, pMessage, object);
    }
    return VK_FALSE;

}

}
