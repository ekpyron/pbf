/**
 *
 *
 * @file MemoryManager.h
 * @brief 
 * @author clonker
 * @date 1/23/19
 */
#pragma once

#include "common.h"

namespace pbf {

class DeviceMemory;
class Buffer;

class HeapManager {
public:
    HeapManager(ContextInterface* context, std::uint32_t memoryTypeIndex);

    HeapManager(const HeapManager&) = delete;
    HeapManager &operator=(const HeapManager &) = delete;
    HeapManager(HeapManager &&) noexcept = default;
    HeapManager&operator=(HeapManager &&) noexcept = default;

    DeviceMemory allocate(std::size_t size, std::size_t alignment, bool hostVisible);
    void free(const DeviceMemory &);

	ContextInterface* context() { return _context; }
private:
    std::uint32_t _memoryTypeIndex;
    //std::vector<vk::DeviceMemory> _blocks;
	ContextInterface* _context;
};

class DeviceMemory {
public:
    DeviceMemory() = default;

    DeviceMemory(std::size_t size, vk::DeviceMemory memory, std::size_t offset, HeapManager *heapManager, bool hostVisible);

    DeviceMemory(const DeviceMemory &) = delete;
    DeviceMemory &operator=(const DeviceMemory &) = delete;
    DeviceMemory(DeviceMemory &&rhs) noexcept {
        *this = std::move(rhs);
    }
    DeviceMemory &operator=(DeviceMemory &&rhs) noexcept {
        if(_mgr) {
            _mgr->free(*this);
        }
        _memory = rhs._memory;
        _offset = rhs._offset;
        _size = rhs._size;
        _mgr = rhs._mgr;
        ptr = rhs.ptr;
        rhs._size = 0;
        rhs._mgr = nullptr;
        rhs.ptr = nullptr;
        return *this;
    }

    ~DeviceMemory() {
        if(_mgr) {
            _mgr->free(*this);
        }
    }

    auto size() const { return _size; }
    auto offset() const { return _offset; }
    auto vkMemory() const { return _memory; }

    void* data() { return ptr; }
    const void* data() const { return ptr; }

    void flush();

private:
    vk::DeviceMemory _memory {};
    std::size_t _offset {0};
    std::size_t _size {0};
    HeapManager *_mgr {nullptr};
    void* ptr {nullptr};
};

class MemoryManager {

public:
    explicit MemoryManager(ContextInterface *context);

    MemoryManager(const MemoryManager &) = delete;
    MemoryManager& operator=(const MemoryManager &) = delete;

    DeviceMemory allocateBufferMemory(MemoryType memoryType, vk::Buffer buffer);
    DeviceMemory allocateImageMemory(MemoryType memoryType, vk::Image buffer);

private:

    std::uint32_t chooseAdequateMemoryIndex(MemoryType type, std::uint32_t memoryTypeBitRequirements);

    DeviceMemory allocate(std::uint32_t memoryTypeIndex, std::size_t size, std::size_t alignment, bool hostVisible) {
        return _memoryHeaps.at(memoryTypeIndex).allocate(size, alignment, hostVisible);
    }

	ContextInterface * _context;

    std::vector<HeapManager> _memoryHeaps;
    std::vector<vk::MemoryType> _memoryTypes;
};

}
