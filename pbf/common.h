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
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <crampl/crampl.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <vulkan/vulkan.hpp>

namespace pbf {
class ContextInterface;
class Context;
class InitContext;
class Renderer;
class MemoryManager;
class Scene;
class Selection;

template<typename Container>
std::uint32_t size32(Container const& _container) {
	return static_cast<std::uint32_t>(_container.size());
}


namespace descriptors {
class GraphicsPipeline;
class RenderPass;
class DescriptorSetLayout;
}

enum class MemoryType {
    STATIC, TRANSIENT, DYNAMIC
};

}

template <>
struct fmt::formatter<pbf::MemoryType>: fmt::formatter<std::string_view> {
	template <typename FormatContext>
	auto format(pbf::MemoryType p, FormatContext& ctx) const {
		return formatter<string_view>::format([](auto p) {
			switch (p) {
				case pbf::MemoryType::STATIC:
					return "STATIC";
				case pbf::MemoryType::TRANSIENT:
					return "TRANSIENT";
				case pbf::MemoryType::DYNAMIC:
					return "DYNAMIC";
			}
			return "UNKNOWN";
		}(p), ctx);
	}
};

namespace pbf {

inline std::ostream& operator<<(std::ostream &os, const MemoryType& memoryType) {
    switch (memoryType) {
        case MemoryType::STATIC: { os << "STATIC"; break; }
        case MemoryType::TRANSIENT:{ os << "TRANSIENT"; break; }
        case MemoryType::DYNAMIC:{ os << "DYNAMIC"; break; }
    }
    return os;
}

struct FrameDataBase {
	virtual ~FrameDataBase() = default;
};

template<typename T>
struct FrameData: FrameDataBase {
	template<typename... Args>
	FrameData(Args&&... args): data(std::forward<Args>(args)...) {}
	FrameData(FrameData const&) = delete;
	FrameData& operator=(FrameData const&) = delete;
	T data;
};

}

#ifndef NDEBUG
#define PBF_DESC_DEBUG_NAME(name) .debugName = name
#else
#define PBF_DESC_DEBUG_NAME(name)
#endif