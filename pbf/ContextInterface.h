#pragma once

#include <vulkan/vulkan.hpp>

namespace pbf {

class MemoryManager;

class ContextInterface {
public:
	virtual const vk::Device &device() const = 0;
	virtual const vk::PhysicalDevice &physicalDevice() const = 0;
	virtual MemoryManager &memoryManager() = 0;
#ifndef NDEBUG
	virtual void setGenericObjectName(vk::ObjectType type, uint64_t obj, const std::string &name) const = 0;
#endif

};

}