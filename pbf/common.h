/**
 *
 *
 * @file common.h
 * @brief 
 * @author clonker
 * @date 9/26/18
 */
#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <vulkan/vulkan.hpp>

namespace pbf {
class Context;
class Renderer;

namespace descriptors {
class GraphicsPipeline;
}
}

#ifndef NDEBUG
#define PBF_DESC_DEBUG_NAME(name) .debugName = name
#else
#define PBF_DESC_DEBUG_NAME(name)
#endif