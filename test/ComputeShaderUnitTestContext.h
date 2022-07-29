#pragma once

#include <vulkan/vulkan.hpp>
#include <pbf/common.h>
#include <pbf/ContextInterface.h>
#include <pbf/Cache.h>
#include <pbf/MemoryManager.h>
#include <pbf/descriptors/ComputePipeline.h>
#include <pbf/Buffer.h>
#include <map>

namespace pbf::test {

class ComputeShaderUnitTestContext : public ContextInterface {
public:
	ComputeShaderUnitTestContext();
	ComputeShaderUnitTestContext(const ComputeShaderUnitTestContext&) = delete;
	ComputeShaderUnitTestContext& operator=(const ComputeShaderUnitTestContext&) = delete;

	const vk::Device &device() const override { return *_device; }
	const vk::PhysicalDevice &physicalDevice() const override { return _physicalDevice; }
	MemoryManager &memoryManager() override { return *_memoryManager; }

#ifndef NDEBUG
	void setGenericObjectName(vk::ObjectType type, uint64_t obj, const std::string &name) const override {
		_device->setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
			.objectType = type,
			.objectHandle = obj,
			.pObjectName = name.c_str()
		}, *_dldi);
	}
#endif

	Cache& cache() {
		return _cache;
	}
	const Cache& cache() const {
		return _cache;
	}

	template<typename Callable>
	void run(Callable _callable) {
		_commandBuffer.reset();
		_commandBuffer.begin(vk::CommandBufferBeginInfo{
			.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
		});
		_callable(_commandBuffer);
		_commandBuffer.end();

		_queue.submit({vk::SubmitInfo{
			.commandBufferCount = 1,
			.pCommandBuffers = &_commandBuffer
		}});
		_queue.waitIdle();

		_device->waitIdle();
	}

	static descriptors::ShaderModule::RawSPIRV compileComputeShader(const std::string& _source);

	std::uint32_t queueFamilyIndex() const {
		return _queueFamilyIndex;
	}
private:
	vk::DispatchLoaderStatic _dls;
	vk::UniqueInstance _instance;
	std::unique_ptr<vk::DispatchLoaderDynamic> _dldi;
	vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> _debugUtilsMessenger;
	vk::PhysicalDevice _physicalDevice;
	std::uint32_t _queueFamilyIndex = -1u;
	vk::UniqueDevice _device;
	vk::Queue _queue;
	Cache _cache { this, 100 };

	vk::UniqueCommandPool _commandPool;
	vk::CommandBuffer _commandBuffer;
	std::unique_ptr<MemoryManager> _memoryManager;

	std::tuple<vk::PhysicalDevice, std::uint32_t> getPhysicalDevice();
};

}
