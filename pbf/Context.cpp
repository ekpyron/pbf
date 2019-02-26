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
#include "Context.h"
#include "Renderer.h"
#include "Scene.h"

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
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
                                             }, nullptr, dls);
    }
#ifndef NDEBUG
    dldi = std::make_unique<vk::DispatchLoaderDynamic>(*_instance, vkGetInstanceProcAddr);
    _debugUtilsMessenger = _instance->createDebugUtilsMessengerEXTUnique(
            vk::DebugUtilsMessengerCreateInfoEXT {
                    {}, ~vk::DebugUtilsMessageSeverityFlagBitsEXT(), ~vk::DebugUtilsMessageTypeFlagBitsEXT(),
                    [](VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
                       VkDebugUtilsMessageTypeFlagsEXT                  messageType,
                       const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
                       void*                                            pUserData) -> VkBool32 {
                        return static_cast<Context *>(pUserData)->debugUtilMessengerCallback(
                                vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity),
                                vk::DebugUtilsMessageTypeFlagBitsEXT(messageType),
                                *reinterpret_cast<const vk::DebugUtilsMessengerCallbackDataEXT*>(pCallbackData)
                                );
                        },
                       this
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
        features.setMultiDrawIndirect(static_cast<vk::Bool32>(true));
        std::vector<const char *> layers;
#ifndef NDEBUG
        layers.push_back("VK_LAYER_LUNARG_standard_validation");
#endif
        auto extensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        _device = _physicalDevice.createDeviceUnique(
                {{}, static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(),
                 static_cast<uint32_t>(layers.size()), layers.data(), 1, &extensionName, &features});
        PBF_DEBUG_SET_OBJECT_NAME(this, _physicalDevice, "Physical Device");
        PBF_DEBUG_SET_OBJECT_NAME(this, *_surface, "Main Window");
        PBF_DEBUG_SET_OBJECT_NAME(this, *_debugUtilsMessenger, "Debug Utils Messenger");
        PBF_DEBUG_SET_OBJECT_NAME(this, *_device, "Main Device");

        _graphicsQueue = _device->getQueue(_families.graphics, 0);
        if (_families.graphics != _families.present) {
            PBF_DEBUG_SET_OBJECT_NAME(this, _graphicsQueue, "Graphics Queue");
            _presentQueue = _device->getQueue(_families.present, 0);
            PBF_DEBUG_SET_OBJECT_NAME(this, _presentQueue, "Present Queue");

        } else {
            _presentQueue = _graphicsQueue;
            PBF_DEBUG_SET_OBJECT_NAME(this, _presentQueue, "Graphics & Present Queue");
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
        auto it = std::find(std::begin(modes), std::end(modes), vk::PresentModeKHR::eMailbox);
        if(it != std::end(modes)) _presentMode = *it;
    }

    _globalDescriptorPool = _device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
        {}, numGlobalDescriptorSets,
        static_cast<std::uint32_t>(globalDescriptorPoolSizes().size()), globalDescriptorPoolSizes().data()});

    {
        _globalDescriptorSetLayout = cache().fetch(descriptors::DescriptorSetLayout{
                .createFlags = {},
                .bindings = {{
                                     .binding = 0,
                                     .descriptorType = vk::DescriptorType::eUniformBuffer,
                                     .descriptorCount = 1,
                                     .stageFlags = vk::ShaderStageFlagBits::eAll
                             }}
#ifndef NDEBUG
                ,.debugName = "Global Descriptor Set Layout"
#endif
        });
/*
        for(auto [setLayout, descriptor] : crampl::ContainerContainer(_globalDescriptorSetLayouts,
                                                                      setLayoutDescriptors)) {
            setLayout = descriptor.realize(this);
        }

        std::array<vk::DescriptorSetLayout, numGlobalDescriptorSets> globalDescriptorSetLayouts;
        std::transform(_globalDescriptorSetLayouts.begin(), _globalDescriptorSetLayouts.end(),
                globalDescriptorSetLayouts.begin(), [](const auto& f) {return *f;});*/
        _globalDescriptorSet = _device->allocateDescriptorSets({*_globalDescriptorPool, numGlobalDescriptorSets,
                                                                 &*_globalDescriptorSetLayout}).front();

    }

    _memoryManager = std::make_unique<MemoryManager>(this);

    {
        _globalUniformBuffer = std::make_unique<Buffer>(this, sizeof(glm::mat4), vk::BufferUsageFlagBits::eUniformBuffer, MemoryType::DYNAMIC);
        vk::DescriptorBufferInfo descriptorBufferInfo {
                _globalUniformBuffer->buffer(), 0, sizeof(glm::mat4)
        };
        *_globalUniformBuffer->as<glm::mat4>() = glm::mat4(1);
        _device->updateDescriptorSets({vk::WriteDescriptorSet{
                _globalDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptorBufferInfo, nullptr
        }}, {});
    }


    _commandPool = _device->createCommandPoolUnique({{}, _families.graphics});
    _commandPoolTransient = _device->createCommandPoolUnique({vk::CommandPoolCreateFlagBits::eTransient,
                                                              _families.graphics});
    PBF_DEBUG_SET_OBJECT_NAME(this, *_commandPool, "Main Command Pool");
    _renderer = std::make_unique<Renderer>(this);
    _scene = std::make_unique<Scene>(this);
}

Context::~Context() = default;

#ifndef NDEBUG
VkBool32
Context::debugUtilMessengerCallback(vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT &callbackData) const {
    auto logger = spdlog::get("vulkan");
    std::string messageTypeString = "Unknown";
    std::map<std::string, std::string> nameMap;
    std::string message = callbackData.pMessage;
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
    } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        logger->warn("[{}] {}", messageTypeString, message);
    } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
        logger->info("[{}] {}", messageTypeString, message);
    } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose) {
        logger->debug("[{}] {}", messageTypeString, message);
    }
    return VK_FALSE;
}

void Context::setGenericObjectName(vk::ObjectType type, uint64_t obj, const std::string &name) const {
    //spdlog::get("vulkan")->info("Assign name [{}] = object {:#x}", name, obj);
    _device->setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
            type, obj, name.c_str()
    }, *dldi);
}
#endif

void Context::run() {
    spdlog::get("console")->debug("Entering main loop.");
    while (!_window->shouldClose()) {
        _glfw.pollEvents();
        _globalDescriptorSetLayout.keepAlive();
        _renderer->render();
        _cache.frame();
    }
    spdlog::get("console")->debug("Exiting main loop. Waiting for idle device.");
    _device->waitIdle();
    spdlog::get("console")->debug("Returning from main loop.");
}

}
