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
    Buffer() = default;
    Buffer(Context* context, std::size_t size, vk::BufferUsageFlags usageFlags, MemoryType memoryType,
            const std::vector<std::uint32_t> &queueFamilyIndices = {});
    ~Buffer();

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer&) = delete;
    Buffer(Buffer &&rhs) noexcept { *this = std::move(rhs); };
    Buffer &operator=(Buffer &&) noexcept;

    void* data() { return _deviceMemory.data(); }
    const void* data() const { return _deviceMemory.data(); }
    std::size_t size() const { return _size; }
    vk::Buffer buffer() const { return _buffer; }

    void flush() {
        _deviceMemory.flush();
    };

    bool operator<(const Buffer& rhs) const {
        return _buffer < rhs._buffer;
    }

private:
    Context *context {nullptr};
    std::size_t _size {0};
    vk::Buffer _buffer {};
    DeviceMemory _deviceMemory {};
};

struct BufferRef {
    Buffer* buffer;
    bool operator<(const BufferRef& rhs) const {
        return (buffer && rhs.buffer) ? *buffer < *rhs.buffer : buffer < rhs.buffer;
    }
};

}
