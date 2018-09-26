#pragma once

#include <vulkan/vulkan.hpp>
#include "common.h"
#include "glfw.h"

namespace pbf {

class Context {
public:
    Context() {
        if(!_glfw.vulkanSupported()) {
            throw std::runtime_error("Vulkan not supported");
        }
        _glfw.windowHint(GLFW_CLIENT_API, GLFW_NO_API);
        _window = std::make_unique<glfw::Window>(700, 700, "PBF", nullptr, nullptr);

        std::vector<const char*> layers;
        auto extensions = _glfw.getRequiredInstanceExtensions();
#ifndef NDEBUG
        extensions.push_back("VK_EXT_debug_report");
        //layers.push_back("VK_LAYER_LUNARG_standard_validation"); //leads to crash
#endif
        vk::ApplicationInfo appInfo {
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

#ifndef NDEBUG
        auto debugFlags = vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eInformation | vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eWarning;
        auto callback = [](VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, std::uint64_t object, std::size_t location, std::int32_t messageCode, const char *pLayerPrefix, const char* pMessage, void *userData) -> VkBool32 {
            return static_cast<Context*>(userData)->debugReportCallback(flags, objectType, object, location, messageCode, pLayerPrefix, pMessage);
        };
        dldi = std::make_unique<vk::DispatchLoaderDynamic>(*_instance);
        _debugReportCallback = _instance->createDebugReportCallbackEXTUnique(vk::DebugReportCallbackCreateInfoEXT{
                debugFlags, callback, this
        }, nullptr, *dldi);
#endif

        _surface = _window->createSurface(*_instance);
    }

    void run() {
        while(!_window->shouldClose()) {
            _glfw.pollEvents();
        }
    }
private:

    VkBool32 debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, std::uint64_t object, std::size_t location, std::int32_t messageCode, const char *pLayerPrefix, const char* pMessage) {
        auto logger = spdlog::get("console");
        if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
            logger->critical("Vulkan ({}): \"{}\", Object: {:#x}", pLayerPrefix, pMessage, object);
        } else if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
            logger->warn("Vulkan ({}): \"{}\", Object: {:#x}", pLayerPrefix, pMessage, object);
        } else if(flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
            logger->warn("Vulkan performance ({}): \"{}\", Object: {:#x}", pLayerPrefix, pMessage, object);
        } else if(flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
            logger->debug("Vulkan ({}): \"{}\", Object: {:#x}", pLayerPrefix, pMessage, object);
        } else if(flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
            logger->info("Vulkan ({}): \"{}\", Object: {:#x}", pLayerPrefix, pMessage, object);
        }
        return VK_FALSE;
    }

    glfw::GLFW _glfw;
    std::unique_ptr<glfw::Window> _window;

    vk::UniqueInstance _instance {nullptr};
#ifndef NDEBUG
    std::unique_ptr<vk::DispatchLoaderDynamic> dldi;
    vk::UniqueHandle<vk::DebugReportCallbackEXT, vk::DispatchLoaderDynamic> _debugReportCallback;
#endif
    vk::UniqueSurfaceKHR _surface;
    vk::PhysicalDevice _physicalDevice;
};

}
