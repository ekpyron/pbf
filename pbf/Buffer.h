/**
 *
 *
 * @file Buffer.h
 * @brief 
 * @author clonker
 * @date 1/23/19
 */
#pragma once

#include "common.h"
#include "MemoryManager.h"

namespace pbf {
class Buffer {

public:
    Buffer(Context* context, std::size_t size, vk::BufferUsageFlags usageFlags, MemoryType memoryType,
            const std::vector<std::uint32_t> &queueFamilyIndices = {});
    ~Buffer();

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer&) = delete;
    Buffer(Buffer &&rhs) noexcept { *this = std::move(rhs); };
    Buffer &operator=(Buffer &&) noexcept;

private:
    Context *context;
    std::size_t _size {0};
    vk::Buffer _buffer {};
    DeviceMemory _deviceMemory {};

};
}
