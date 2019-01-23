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

MemoryManager::MemoryManager(Context *context) : _context(context) {

    const auto &physicalDevice  = _context->physicalDevice();

    auto properties = physicalDevice.getMemoryProperties();

    _memoryTypes = {properties.memoryTypes, properties.memoryTypes + properties.memoryTypeCount};

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
            }
        }

    }

    throw std::logic_error(fmt::format("no adequate memory found for type {} and requirements {}",
                                       memoryType, memoryTypeBitRequirements));
}

DeviceMemory MemoryManager::allocateBufferMemory(MemoryType memoryType, vk::Buffer buffer) {
    const auto &device = _context->device();

    auto memoryRequirements = device.getBufferMemoryRequirements(buffer);
    auto memoryIndex = chooseAdequateMemoryIndex(memoryType, memoryRequirements.memoryTypeBits);

    auto deviceMemory = allocate(memoryIndex, memoryRequirements.size, memoryRequirements.alignment);

    device.bindBufferMemory(buffer, deviceMemory.vkMemory(), deviceMemory.offset());

    return std::move(deviceMemory);
}


HeapManager::HeapManager(Context* context, std::uint32_t memoryTypeIndex)
        : _memoryTypeIndex(memoryTypeIndex), _context(context) {}

DeviceMemory HeapManager::allocate(std::size_t size, std::size_t alignment) {
    const auto &device = _context->device();
    auto memory = device.allocateMemory({
        size, _memoryTypeIndex
    });
    return {size, memory, 0, this};
}

void HeapManager::free(const DeviceMemory &deviceMemory) {
    const auto &device = _context->device();
    device.free(deviceMemory.vkMemory());
}


}
