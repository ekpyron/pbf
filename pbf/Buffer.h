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

namespace detail {

std::tuple<vk::Buffer, DeviceMemory> allocateBuffer(
        ContextInterface *context, std::size_t size, vk::BufferUsageFlags usageFlags, MemoryType memoryType,
        const std::vector<uint32_t> &queueFamilyIndices
);

void deallocateBuffer(ContextInterface* context, std::size_t size, vk::Buffer buffer);


}

template<typename T>
class Buffer {
public:
    using value_type = T;

    Buffer() = default;
    Buffer(ContextInterface* context, std::size_t size, vk::BufferUsageFlags usageFlags, MemoryType memoryType,
            const std::vector<std::uint32_t> &queueFamilyIndices = {}) : _size(size), context(context) {
        auto buf = detail::allocateBuffer(context, size * itemSize(), usageFlags, memoryType, queueFamilyIndices);
        _buffer = std::get<0>(buf);
        _deviceMemory = std::move(std::get<1>(buf));
    };
    ~Buffer() {
        detail::deallocateBuffer(context, _size * itemSize(), _buffer);
    };

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer&) = delete;
    Buffer(Buffer &&rhs) noexcept { *this = std::move(rhs); };
    Buffer &operator=(Buffer &&rhs) noexcept {
        _size = rhs._size;
        _buffer = rhs._buffer;
        _deviceMemory = std::move(rhs._deviceMemory);
        context = rhs.context;

        rhs._size = 0;
        rhs.context = nullptr;

        return *this;
    };

    static constexpr std::size_t itemSize() {
        return sizeof(value_type);
    }

    T* data() { return reinterpret_cast<T*>(_deviceMemory.data()); }
    [[nodiscard]] const T* data() const { return reinterpret_cast<T*>(_deviceMemory.data()); }
    [[nodiscard]] std::size_t size() const { return _size; }
    [[nodiscard]] std::size_t deviceSize() const { return _size * itemSize(); }
    [[nodiscard]] vk::Buffer buffer() const { return _buffer; }

    void flush() {
        _deviceMemory.flush();
    };

    bool operator<(const Buffer& rhs) const {
        return _buffer < rhs._buffer;
    }

private:
	ContextInterface *context {nullptr};
    std::size_t _size {0};
    vk::Buffer _buffer {};
    DeviceMemory _deviceMemory {};
};

template<typename T>
struct BufferRef {
	Buffer<T>* buffer = nullptr;
	size_t offset = 0;
	bool operator<(const BufferRef& rhs) const {
		return
			((buffer && rhs.buffer) ? *buffer < *rhs.buffer : buffer < rhs.buffer) &&
			offset < rhs.offset;
	}
};

}
