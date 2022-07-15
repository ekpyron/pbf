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

pbf::Buffer::Buffer(pbf::ContextInterface *context, std::size_t size, vk::BufferUsageFlags usageFlags, MemoryType memoryType,
                    const std::vector<uint32_t> &queueFamilyIndices) : _size(size), context(context) {

    const auto &device = context->device();
    auto &mmgr = context->memoryManager();
    _buffer = device.createBuffer(vk::BufferCreateInfo{
		.size = size,
		.usage = usageFlags,
		.sharingMode = queueFamilyIndices.empty() ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
		.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size()),
		.pQueueFamilyIndices = queueFamilyIndices.data()
	});
    _deviceMemory = mmgr.allocateBufferMemory(memoryType, _buffer);
}

pbf::Buffer &pbf::Buffer::operator=(pbf::Buffer &&rhs) noexcept {

    _size = rhs._size;
    _buffer = rhs._buffer;
    _deviceMemory = std::move(rhs._deviceMemory);
    context = rhs.context;

    rhs._size = 0;
    rhs.context = nullptr;

    return *this;
}

pbf::Buffer::~Buffer() {
    if(_size > 0) {
        const auto &device = context->device();

        device.destroyBuffer(_buffer);
    }
}

