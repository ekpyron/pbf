/**
 *
 *
 * @file common.h
 * @brief 
 * @author clonker
 * @date 9/26/18
 */
#pragma once

#include <ostream>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <vulkan/vulkan.hpp>

namespace pbf {
class Context;
class Renderer;
class MemoryManager;
class Scene;

namespace descriptors {
class GraphicsPipeline;
}

enum class MemoryType {
    STATIC, TRANSIENT, DYNAMIC
};

inline std::ostream& operator<<(std::ostream &os, const MemoryType& memoryType) {
    switch (memoryType) {
        case MemoryType::STATIC: { os << "STATIC"; break; }
        case MemoryType::TRANSIENT:{ os << "TRANSIENT"; break; }
        case MemoryType::DYNAMIC:{ os << "DYNAMIC"; break; }
    }
    return os;
}

}

#ifndef NDEBUG
#define PBF_DESC_DEBUG_NAME(name) .debugName = name
#else
#define PBF_DESC_DEBUG_NAME(name)
#endif