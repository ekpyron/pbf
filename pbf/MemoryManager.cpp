/**
 *
 *
 * @file MemoryManager.cpp
 * @brief 
 * @author clonker
 * @date 1/23/19
 */
#include "MemoryManager.h"

#include "Context.h"

namespace pbf {

MemoryManager::MemoryManager(ContextInterface &context) : _context(context) {

    const auto &physicalDevice  = _context.physicalDevice();

    auto properties = physicalDevice.getMemoryProperties();

    _memoryTypes = {properties.memoryTypes.data(), properties.memoryTypes.data() + properties.memoryTypeCount};

    _memoryHeaps.reserve(properties.memoryTypeCount);
    for(std::uint32_t i = 0; i < properties.memoryTypeCount; ++i) { _memoryHeaps.emplace_back(context, i); }
}

std::uint32_t
MemoryManager::chooseAdequateMemoryIndex(MemoryType memoryType, uint32_t memoryTypeBitRequirements) {
    for(std::uint32_t memoryIndex = 0; memoryIndex < _memoryTypes.size(); ++memoryIndex) {
        auto memoryTypeBits = (1 << memoryIndex);
        if (memoryTypeBitRequirements & memoryTypeBits) {
            const auto &type = _memoryTypes[memoryIndex];

            switch (memoryType) {
                case MemoryType::STATIC: {
                    if(type.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) {
                        return memoryIndex;
                    }
                    break;
                }
                case MemoryType::TRANSIENT: {
                    if(type.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) {
                        return memoryIndex;
                    }
                    break;
                }
                case MemoryType::DYNAMIC: {
                    if((type.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) && (type.propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent)) {
                        return memoryIndex;
                    }
                    break;
                }
            }
        }

    }

    throw std::logic_error(fmt::format("no adequate memory found for type {} and requirements {}",
                                       memoryType, memoryTypeBitRequirements));
}

DeviceMemory MemoryManager::allocateBufferMemory(MemoryType memoryType, vk::Buffer buffer) {
    const auto &device = _context.device();

    auto memoryRequirements = device.getBufferMemoryRequirements(buffer);
    auto memoryIndex = chooseAdequateMemoryIndex(memoryType, memoryRequirements.memoryTypeBits);

    auto deviceMemory = allocate(memoryIndex, memoryRequirements.size, memoryRequirements.alignment,
            static_cast<bool>(_memoryTypes[memoryIndex].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible));

    device.bindBufferMemory(buffer, deviceMemory.vkMemory(), deviceMemory.offset());

    return std::move(deviceMemory);
}

DeviceMemory MemoryManager::allocateImageMemory(MemoryType memoryType, vk::Image image) {
	const auto &device = _context.device();

	auto memoryRequirements = device.getImageMemoryRequirements(image);
	auto memoryIndex = chooseAdequateMemoryIndex(memoryType, memoryRequirements.memoryTypeBits);

	auto deviceMemory = allocate(memoryIndex, memoryRequirements.size, memoryRequirements.alignment,
								 static_cast<bool>(_memoryTypes[memoryIndex].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible));

	device.bindImageMemory(image, deviceMemory.vkMemory(), deviceMemory.offset());

	return std::move(deviceMemory);
}


HeapManager::HeapManager(ContextInterface &context, std::uint32_t memoryTypeIndex)
        : _memoryTypeIndex(memoryTypeIndex), _context(context) {}

DeviceMemory HeapManager::allocate(std::size_t size, std::size_t alignment, bool hostVisible) {
    const auto &device = _context.device();
    auto memory = device.allocateMemory(vk::MemoryAllocateInfo{
		.allocationSize = size,
		.memoryTypeIndex = _memoryTypeIndex
    });
    return {size, memory, 0, this, hostVisible};
}

void HeapManager::free(const DeviceMemory &deviceMemory) {
    const auto &device = _context.device();
    device.free(deviceMemory.vkMemory());
}


DeviceMemory::DeviceMemory(std::size_t size, vk::DeviceMemory memory, std::size_t offset, HeapManager *heapManager, bool hostVisible)
        : _size(size), _memory(memory), _offset(offset), _mgr(heapManager){
    const auto &device = _mgr->context().device();
    if (hostVisible) {
        ptr = device.mapMemory(vkMemory(), offset, size, {});
    }
}

void DeviceMemory::flush() {
    const auto &device = _mgr->context().device();

    device.flushMappedMemoryRanges({vk::MappedMemoryRange{
		.memory = _memory,
		.offset = _offset,
		.size = _size
	}});
}
}
