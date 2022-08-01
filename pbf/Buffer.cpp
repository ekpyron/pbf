/**
 *
 *
 * @file Buffer.cpp
 * @brief 
 * @author clonker
 * @date 1/23/19
 */
#include "Buffer.h"
#include "Context.h"

std::tuple<vk::Buffer, pbf::DeviceMemory> pbf::detail::allocateBuffer(
        pbf::ContextInterface *context, std::size_t size, vk::BufferUsageFlags usageFlags, MemoryType memoryType,
        const std::vector<uint32_t> &queueFamilyIndices) {
    const auto &device = context->device();
    auto &mmgr = context->memoryManager();
    auto _buffer = device.createBuffer(vk::BufferCreateInfo{
            .size = size,
            .usage = usageFlags,
            .sharingMode = queueFamilyIndices.empty() ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
            .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size()),
            .pQueueFamilyIndices = queueFamilyIndices.data()
    });
    auto _deviceMemory = mmgr.allocateBufferMemory(memoryType, _buffer);
    return std::make_tuple(_buffer, std::move(_deviceMemory));
};

void pbf::detail::deallocateBuffer(pbf::ContextInterface* context, std::size_t size, vk::Buffer buffer) {
    if(size > 0) {
        const auto &device = context->device();
        device.destroyBuffer(buffer);
    }
};
