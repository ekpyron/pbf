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
#include "Selection.h"
#include "GUI.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_projection.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace pbf {


Context::Context() {
    if (!_glfw.vulkanSupported()) {
        throw std::runtime_error("Vulkan not supported");
    }
    _glfw.windowHint(GLFW_CLIENT_API, GLFW_NO_API);
	/*
	auto* monitor = glfwGetPrimaryMonitor();
	auto* mode = glfwGetVideoMode(monitor);
	_glfw.windowHint(GLFW_RED_BITS, mode->redBits);
	_glfw.windowHint(GLFW_GREEN_BITS, mode->greenBits);
	_glfw.windowHint(GLFW_BLUE_BITS, mode->blueBits);
	_glfw.windowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    _window = std::make_unique<glfw::Window>(mode->width, mode->height, "PBF", monitor, nullptr);
	*/
	_window = std::make_unique<glfw::Window>(768*2, 768, "PBF", nullptr, nullptr);

    {
        std::vector<const char *> layers;
        auto extensions = _glfw.getRequiredInstanceExtensions();
#ifndef NDEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
        vk::ApplicationInfo appInfo{
                .pApplicationName = "PBF",
                .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
                .pEngineName = "PBF",
                .engineVersion = VK_MAKE_VERSION(0, 0, 0),
                .apiVersion = VK_API_VERSION_1_1
        };
#ifndef NDEBUG
		vk::ValidationFeatureEnableEXT enables[] = {
			vk::ValidationFeatureEnableEXT::eGpuAssisted
		};
		vk::ValidationFeaturesEXT validationFeatures{
			.enabledValidationFeatureCount = 1,
			.pEnabledValidationFeatures = enables
		};
#endif
        _instance = vk::createInstanceUnique(vk::InstanceCreateInfo{
#ifndef NDEBUG
			.pNext = &validationFeatures,
#endif
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(layers.size()),
			.ppEnabledLayerNames = layers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data()
		 }, nullptr, dls);
    }
#ifndef NDEBUG
    dldi = std::make_unique<vk::DispatchLoaderDynamic>(*_instance, vkGetInstanceProcAddr);
    _debugUtilsMessenger = _instance->createDebugUtilsMessengerEXTUnique(
            vk::DebugUtilsMessengerCreateInfoEXT {
				.messageSeverity = ~vk::DebugUtilsMessageSeverityFlagBitsEXT(),
				.messageType = ~vk::DebugUtilsMessageTypeFlagBitsEXT(),
				.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
                       VkDebugUtilsMessageTypeFlagsEXT                  messageType,
                       const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
                       void*                                            pUserData) -> VkBool32 {
                        return static_cast<Context *>(pUserData)->debugUtilMessengerCallback(
                                vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity),
                                vk::DebugUtilsMessageTypeFlagBitsEXT(messageType),
                                *reinterpret_cast<const vk::DebugUtilsMessengerCallbackDataEXT*>(pCallbackData)
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
        std::vector<const char *> layers;
#ifndef NDEBUG
        layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
        auto extensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        _device = _physicalDevice.createDeviceUnique(vk::DeviceCreateInfo{
			.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
			.pQueueCreateInfos = queueCreateInfos.data(),
			.enabledLayerCount = 1,
			.ppEnabledLayerNames = layers.data(),
			.enabledExtensionCount = 1,
			.ppEnabledExtensionNames = &extensionName,
			.pEnabledFeatures = &features
		});
        PBF_DEBUG_SET_OBJECT_NAME(*this, _physicalDevice, "Physical Device");
        PBF_DEBUG_SET_OBJECT_NAME(*this, *_surface, "Main Window");
        //PBF_DEBUG_SET_OBJECT_NAME(*this, *_debugUtilsMessenger, "Debug Utils Messenger");
        PBF_DEBUG_SET_OBJECT_NAME(*this, *_device, "Main Device");

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
        auto it = std::find(std::begin(modes), std::end(modes), vk::PresentModeKHR::eMailbox);
        if(it != std::end(modes)) _presentMode = *it;
    }

	_descriptorPool = _device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
		.maxSets = maxGlobalDescriptorSets,
		.poolSizeCount = static_cast<std::uint32_t>(globalDescriptorPoolSizes().size()),
		.pPoolSizes = globalDescriptorPoolSizes().data()
	});

    {
        _globalDescriptorSetLayout = cache().fetch(descriptors::DescriptorSetLayout{
                .createFlags = {},
                .bindings = {{
                                     .binding = 0,
                                     .descriptorType = vk::DescriptorType::eUniformBuffer,
                                     .descriptorCount = 1,
                                     .stageFlags = vk::ShaderStageFlagBits::eAll
                             }},
			PBF_DESC_DEBUG_NAME("Global Descriptor Set Layout")
        });
/*
        for(auto [setLayout, descriptor] : crampl::ContainerContainer(_globalDescriptorSetLayouts,
                                                                      setLayoutDescriptors)) {
            setLayout = descriptor.realize(this);
        }

        std::array<vk::DescriptorSetLayout, numGlobalDescriptorSets> globalDescriptorSetLayouts;
        std::transform(_globalDescriptorSetLayouts.begin(), _globalDescriptorSetLayouts.end(),
                globalDescriptorSetLayouts.begin(), [](const auto& f) {return *f;});*/
        _globalDescriptorSet = _device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = *_descriptorPool,
			.descriptorSetCount = numGlobalDescriptorSets,
			.pSetLayouts = &*_globalDescriptorSetLayout,
		}).front();

    }

    _memoryManager = std::make_unique<MemoryManager>(*this);

    _commandPool = _device->createCommandPoolUnique(vk::CommandPoolCreateInfo{.queueFamilyIndex = _families.graphics});
    _commandPoolTransient = _device->createCommandPoolUnique(vk::CommandPoolCreateInfo{
		.flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = _families.graphics
	});
    PBF_DEBUG_SET_OBJECT_NAME(*this, *_commandPool, "Main Command Pool");

	InitContext initContext(*this);

	initContext.initCommandBuffer->begin(vk::CommandBufferBeginInfo{
		.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
		.pInheritanceInfo = nullptr
	});

    _renderer = std::make_unique<Renderer>(initContext);
    _scene = std::make_unique<Scene>(initContext);
	_gui = std::make_unique<GUI>(initContext);

	initContext.initCommandBuffer->end();


	{
		_globalUniformBuffer = std::make_unique<Buffer<GlobalUniformData>>(*this, 1, vk::BufferUsageFlagBits::eUniformBuffer, MemoryType::DYNAMIC);
		vk::DescriptorBufferInfo uniformBufferDescriptorInfo {
			_globalUniformBuffer->buffer(), 0, sizeof(GlobalUniformData)
		};
		globalUniformData = _globalUniformBuffer->data();
		_device->updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = _globalDescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &uniformBufferDescriptorInfo,
			.pTexelBufferView = nullptr
		}}, {});
	}

	_graphicsQueue.submit({
							  vk::SubmitInfo{
								  .waitSemaphoreCount = 0,
								  .pWaitSemaphores = nullptr,
								  .pWaitDstStageMask = {},
								  .commandBufferCount = 1,
								  .pCommandBuffers = &*initContext.initCommandBuffer,
								  .signalSemaphoreCount = 0, // TODO: replace waitIdle below with a semaphore
								  .pSignalSemaphores = nullptr
							  }
	});
	_graphicsQueue.waitIdle();

	_gui->postInitCleanup();
}

Context::~Context()
{
}

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
		.objectType = type,
		.objectHandle = obj,
		.pObjectName = name.c_str()
    }, *dldi);
}
#endif

void Context::run() {
    spdlog::get("console")->debug("Entering main loop.");
	float rot = 0.0f;
	double lastTime = glfwGetTime();
	_camera.SetPosition(glm::vec3 (0, 0, -100));
    while (!_window->shouldClose()) {
		double now = glfwGetTime();
		double timePassed = now - lastTime;
		lastTime = now;
        _glfw.pollEvents();
		auto [width, height] = window().framebufferSize();
		glm::mat4x4 clip = glm::mat4x4( 1.0f,  0.0f, 0.0f, 0.0f,
									    0.0f, -1.0f, 0.0f, 0.0f,
									    0.0f,  0.0f, 0.5f, 0.0f,
									    0.0f,  0.0f, 0.5f, 1.0f);
		glm::mat4 projmat = glm::perspective(glm::radians(60.0f), float(width) / float(height), 0.1f, 1000.0f);
		glm::mat4 mvmat = _camera.GetViewMatrix(); // glm::rotate(glm::translate(glm::mat4(1), glm::vec3(0,0,-3)), rot, glm::vec3(0,0,1));
		rot += 1.0f * timePassed;
		globalUniformData->mvpmatrix = clip * projmat * mvmat;
		globalUniformData->viewrot = _camera.GetViewRot();
        _globalDescriptorSetLayout.keepAlive();
        _renderer->render();
        _cache.frame();
    }
    spdlog::get("console")->debug("Exiting main loop. Waiting for idle device.");
    _device->waitIdle();
    spdlog::get("console")->debug("Returning from main loop.");
}

vk::Format Context::depthFormat() const
{
	// TODO
	return vk::Format::eD32Sfloat;
}

}
