#include <spdlog/spdlog.h>
#include <map>
#include <string>
#include <regex>
#include <spdlog/sinks/base_sink.h>
#include <catch2/catch_test_macros.hpp>
#include "ComputeShaderUnitTestContext.h"
#include <glslang/Include/glslang_c_interface.h>

auto debugUtilMessengerCallback(vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT &callbackData) {
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
		WARN(fmt::format("[ERROR] [{}] {}", messageTypeString, message));
    } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
		WARN(fmt::format("[WARNING] [{}] {}", messageTypeString, message));
    } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
		UNSCOPED_INFO(fmt::format("[INFO] [{}] {}", messageTypeString, message));
    } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose) {
		UNSCOPED_INFO(fmt::format("[VERBOSE] [{}] {}", messageTypeString, message));
    }
    return VK_FALSE;
}

namespace pbf::test {

class Catch2Sink : public spdlog::sinks::base_sink<std::mutex> {
public:
protected:
	void sink_it_(const spdlog::details::log_msg& msg) override {
		spdlog::memory_buf_t formatted;
		formatter_->format(msg, formatted);
		UNSCOPED_INFO(std::string(formatted.data(), formatted.size()));
	}
	void flush_() override {}
};

struct Loggers {
	Loggers() {
		auto sink = std::make_shared<Catch2Sink>();
		auto loggerVulkan = std::make_shared<spdlog::logger>("vulkan", sink);
		loggerVulkan->set_pattern("[%T.%f] %^[%l] [Vulkan] %v%$");
		loggerVulkan->set_level(spdlog::level::debug);
		spdlog::register_logger(loggerVulkan);
		auto consoleLogger = std::make_shared<spdlog::logger>("console", sink);
		consoleLogger->set_pattern("[%T.%f] %^[%l] %v%$");
		consoleLogger->set_level(spdlog::level::debug);
		spdlog::register_logger(consoleLogger);
	}
};

ComputeShaderUnitTestContext::ComputeShaderUnitTestContext() {
	static Loggers loggers;

	auto extensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
	auto layers = {"VK_LAYER_KHRONOS_validation"};

	vk::ApplicationInfo appInfo{
		.pApplicationName = "PBF Compute Shader Unit Tests",
		.applicationVersion = VK_MAKE_VERSION(0, 0, 0),
		.pEngineName = "PBF",
		.engineVersion = VK_MAKE_VERSION(0, 0, 0),
		.apiVersion = VK_API_VERSION_1_1
	};
	_instance = vk::createInstanceUnique(vk::InstanceCreateInfo{
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(layers.size()),
		.ppEnabledLayerNames = &*layers.begin(),
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = &*extensions.begin()
	}, nullptr, _dls);

	_dldi = std::make_unique<vk::DispatchLoaderDynamic>(*_instance, vkGetInstanceProcAddr);
	_debugUtilsMessenger = _instance->createDebugUtilsMessengerEXTUnique(
		vk::DebugUtilsMessengerCreateInfoEXT {
			.messageSeverity = ~vk::DebugUtilsMessageSeverityFlagBitsEXT(),
			.messageType = ~vk::DebugUtilsMessageTypeFlagBitsEXT(),
			.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
								  VkDebugUtilsMessageTypeFlagsEXT                  messageType,
								  const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
								  void*                                            pUserData) -> VkBool32 {
				return debugUtilMessengerCallback(
					vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity),
					vk::DebugUtilsMessageTypeFlagBitsEXT(messageType),
					*reinterpret_cast<const vk::DebugUtilsMessengerCallbackDataEXT*>(pCallbackData)
				);
			},
			.pUserData = nullptr
		}, nullptr, *_dldi);

	auto devices = _instance->enumeratePhysicalDevices();

	std::tie(_physicalDevice, _queueFamilyIndex) = getPhysicalDevice();

	float queuePriority = 1.0f;
	auto queueCreateInfos = {vk::DeviceQueueCreateInfo{
		.queueFamilyIndex  = _queueFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
	}};
	vk::PhysicalDeviceFeatures features{};
	_device = _physicalDevice.createDeviceUnique(vk::DeviceCreateInfo{
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &*queueCreateInfos.begin(),
		.enabledLayerCount = static_cast<uint32_t>(layers.size()),
		.ppEnabledLayerNames = &*layers.begin(),
		.enabledExtensionCount = 0,
		.ppEnabledExtensionNames = nullptr,
		.pEnabledFeatures = &features
	});
	_queue = _device->getQueue(_queueFamilyIndex, 0);
	_memoryManager = std::make_unique<MemoryManager>(this);
	_commandPool = _device->createCommandPoolUnique(vk::CommandPoolCreateInfo{
		.flags = vk::CommandPoolCreateFlagBits::eTransient,
		.queueFamilyIndex = _queueFamilyIndex
	});
	_commandBuffer = device().allocateCommandBuffers(vk::CommandBufferAllocateInfo{
		.commandPool = *_commandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1
	}).front();
}

std::tuple<vk::PhysicalDevice, std::uint32_t> ComputeShaderUnitTestContext::getPhysicalDevice() {
	auto devices = _instance->enumeratePhysicalDevices();
	std::multimap<int, const vk::PhysicalDevice*> ratedDevices;
	for(const auto &physicalDevice : devices) {
		ratedDevices.emplace(physicalDevice.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? 1 : 0, &physicalDevice);
	}
	for (auto it = ratedDevices.rbegin(); it != ratedDevices.rend(); ++it) {
		const auto &physicalDevice = *it->second;
		const auto& queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
		for(std::size_t i = 0; i < queueFamilyProperties.size(); ++i) {
			const auto& qf = queueFamilyProperties[i];
			if(qf.queueFlags & vk::QueueFlagBits::eCompute) {
				return std::make_tuple(physicalDevice, static_cast<std::uint32_t>(i));
			}
		}
	}
	throw std::runtime_error("No suitable discrete GPU found.");
}

descriptors::ShaderModule::RawSPIRV ComputeShaderUnitTestContext::compileComputeShader(const std::string &_source)
{
	// default glslangValidator resource settings
	glslang_resource_t resources {
		.max_lights = 32,
		.max_clip_planes = 6,
		.max_texture_units = 32,
		.max_texture_coords = 32,
		.max_vertex_attribs = 64,
		.max_vertex_uniform_components = 4096,
		.max_varying_floats = 64,
		.max_vertex_texture_image_units = 32,
		.max_combined_texture_image_units = 80,
		.max_texture_image_units = 32,
		.max_fragment_uniform_components = 4096,
		.max_draw_buffers = 32,
		.max_vertex_uniform_vectors = 128,
		.max_varying_vectors = 8,
		.max_fragment_uniform_vectors = 16,
		.max_vertex_output_vectors = 16,
		.max_fragment_input_vectors = 15,
		.min_program_texel_offset = -8,
		.max_program_texel_offset = 7,
		.max_clip_distances = 8,
		.max_compute_work_group_count_x = 65535,
		.max_compute_work_group_count_y = 65535,
		.max_compute_work_group_count_z = 65535,
		.max_compute_work_group_size_x = 1024,
		.max_compute_work_group_size_y = 1024,
		.max_compute_work_group_size_z = 64,
		.max_compute_uniform_components = 1024,
		.max_compute_texture_image_units = 16,
		.max_compute_image_uniforms = 8,
		.max_compute_atomic_counters = 8,
		.max_compute_atomic_counter_buffers = 1,
		.max_varying_components = 60,
		.max_vertex_output_components = 64,
		.max_geometry_input_components = 64,
		.max_geometry_output_components = 128,
		.max_fragment_input_components = 128,
		.max_image_units = 8,
		.max_combined_image_units_and_fragment_outputs = 8,
		.max_combined_shader_output_resources = 8,
		.max_image_samples = 0,
		.max_vertex_image_uniforms = 0,
		.max_tess_control_image_uniforms = 0,
		.max_tess_evaluation_image_uniforms = 0,
		.max_geometry_image_uniforms = 0,
		.max_fragment_image_uniforms = 8,
		.max_combined_image_uniforms = 8,
		.max_geometry_texture_image_units = 16,
		.max_geometry_output_vertices = 256,
		.max_geometry_total_output_components = 1024,
		.max_geometry_uniform_components = 1024,
		.max_geometry_varying_components = 64,
		.max_tess_control_input_components = 128,
		.max_tess_control_output_components = 128,
		.max_tess_control_texture_image_units = 16,
		.max_tess_control_uniform_components = 1024,
		.max_tess_control_total_output_components = 4096,
		.max_tess_evaluation_input_components = 128,
		.max_tess_evaluation_output_components = 128,
		.max_tess_evaluation_texture_image_units = 16,
		.max_tess_evaluation_uniform_components = 1024,
		.max_tess_patch_components = 120,
		.max_patch_vertices = 32,
		.max_tess_gen_level = 64,
		.max_viewports = 16,
		.max_vertex_atomic_counters = 0,
		.max_tess_control_atomic_counters = 0,
		.max_tess_evaluation_atomic_counters = 0,
		.max_geometry_atomic_counters = 0,
		.max_fragment_atomic_counters = 8,
		.max_combined_atomic_counters = 8,
		.max_atomic_counter_bindings = 1,
		.max_vertex_atomic_counter_buffers = 0,
		.max_tess_control_atomic_counter_buffers = 0,
		.max_tess_evaluation_atomic_counter_buffers = 0,
		.max_geometry_atomic_counter_buffers = 0,
		.max_fragment_atomic_counter_buffers = 1,
		.max_combined_atomic_counter_buffers = 1,
		.max_atomic_counter_buffer_size = 16384,
		.max_transform_feedback_buffers = 4,
		.max_transform_feedback_interleaved_components = 64,
		.max_cull_distances = 8,
		.max_combined_clip_and_cull_distances = 8,
		.max_samples = 4,
		.max_mesh_output_vertices_nv = 256,
		.max_mesh_output_primitives_nv = 512,
		.max_mesh_work_group_size_x_nv = 32,
		.max_mesh_work_group_size_y_nv = 1,
		.max_mesh_work_group_size_z_nv = 1,
		.max_task_work_group_size_x_nv = 32,
		.max_task_work_group_size_y_nv = 1,
		.max_task_work_group_size_z_nv = 1,
		.max_mesh_view_count_nv = 4,
		.limits = glslang_limits_t{
			.non_inductive_for_loops = true,
			.while_loops = true,
			.do_while_loops = true,
			.general_uniform_indexing = true,
			.general_attribute_matrix_vector_indexing = true,
			.general_varying_indexing = true,
			.general_sampler_indexing = true,
			.general_variable_indexing = true,
			.general_constant_matrix_vector_indexing = true
		}
	};
	glslang_input_t input = {
		.language = GLSLANG_SOURCE_GLSL,
		.stage = GLSLANG_STAGE_COMPUTE,
		.client = GLSLANG_CLIENT_VULKAN,
		.client_version = GLSLANG_TARGET_VULKAN_1_1,
		.target_language = GLSLANG_TARGET_SPV,
		.target_language_version = GLSLANG_TARGET_SPV_1_3,
		.code = _source.c_str(),
		.default_version = 100,
		.default_profile = GLSLANG_NO_PROFILE,
		.force_default_version_and_profile = false,
		.forward_compatible = false,
		.messages = GLSLANG_MSG_DEFAULT_BIT,
		.resource = &resources
	};

	struct glslangProcess {
		glslangProcess() {
			glslang_initialize_process();
		}
		glslangProcess(const glslangProcess&) = delete;
		glslangProcess& operator=(const glslangProcess&) = delete;
		~glslangProcess() {
			glslang_finalize_process();
		}
	};
	static glslangProcess glslangProcess;

	class glslangShader {
	public:
		glslangShader(glslang_input_t& _input): _shader(glslang_shader_create(&_input)) {}
		glslangShader(const glslangShader&) = delete;
		glslangShader& operator=(const glslangShader&) = delete;
		~glslangShader() { glslang_shader_delete(_shader); }
		operator glslang_shader_t*() { return _shader; }
	private:
		glslang_shader_t* _shader;
	};

	glslangShader shader(input);

	if (!glslang_shader_preprocess(shader, &input)) {
		INFO(glslang_shader_get_info_log(shader));
		INFO(glslang_shader_get_info_debug_log(shader));
		CHECK(false);
	}
	if (!glslang_shader_parse(shader, &input)) {
		INFO(glslang_shader_get_info_log(shader));
		INFO(glslang_shader_get_info_debug_log(shader));
		CHECK(false);
	}

	class glslangProgram {
	public:
		glslangProgram(): _program(glslang_program_create()) {}
		glslangProgram(const glslangProgram&) = delete;
		glslangProgram& operator=(const glslangProgram&) = delete;
		~glslangProgram() { glslang_program_delete(_program); }
		operator glslang_program_t*() { return _program; }
	private:
		glslang_program_t* _program;
	};
	glslangProgram program;
	glslang_program_add_shader(program, shader);

	if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
	{
		INFO(glslang_shader_get_info_log(shader));
		INFO(glslang_shader_get_info_debug_log(shader));
		CHECK(false);
	}

	glslang_program_SPIRV_generate( program, input.stage );

	if (glslang_program_SPIRV_get_messages(program))
	{
		UNSCOPED_INFO(glslang_program_SPIRV_get_messages(program));
	}

	std::vector<std::uint32_t> result{
		glslang_program_SPIRV_get_ptr(program),
		glslang_program_SPIRV_get_ptr(program) + glslang_program_SPIRV_get_size(program)
	};

	return {result};

}
}
