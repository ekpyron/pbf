/**
 *
 *
 * @file Scene.h
 * @brief 
 * @author clonker
 * @date 1/23/19
 */
#pragma once

#include <map>

#include "Context.h"
#include "Buffer.h"
#include "Quad.h"
#include "IndirectCommandsBuffer.h"
#include <crampl/MultiKeyMap.h>
#include <list>
#include "Simulation.h"

namespace pbf {

class Scene {
public:

    Scene(Context* context);

    void frame(vk::CommandBuffer &buf);

    void enqueueCommands(vk::CommandBuffer &buf);

    Context* context() {
        return _context;
    }

    IndirectCommandsBuffer* getIndirectCommandBuffer(const CacheReference<descriptors::GraphicsPipeline> &graphicsPipeline,
													 const std::vector<uint8_t>& pushConstantData,
                                                     const std::tuple<BufferRef, vk::IndexType> &indexBuffer,
                                                     const std::vector<BufferRef> &vertexBuffers)
    {
        auto result = &crampl::emplace(indirectDrawCalls, std::piecewise_construct, std::forward_as_tuple(graphicsPipeline), std::forward_as_tuple(pushConstantData),
                                       std::forward_as_tuple(indexBuffer), std::forward_as_tuple(vertexBuffers), std::forward_as_tuple(_context));
        indirectCommandBuffers.emplace(result);
        return result;
    }

	auto& simulations() { return _simulations; }
	const auto& simulations() const { return _simulations; }
	uint32_t getNumParticles() const {
		return _numParticles;
	}

	Buffer& particleData() { return _particleData; }
	const Buffer& particleData() const { return _particleData; }

	struct ParticleData {
		glm::vec3 position;
		float aux = 0.0f;
	};

private:

    Context* _context;

	uint32_t _numParticles = 64 * 64 * 64;

	Buffer _particleData;

	std::vector<Simulation> _simulations;

	crampl::MultiKeyMap<std::map,
                        CacheReference<descriptors::GraphicsPipeline>,
						std::vector<uint8_t>,
                        std::tuple<BufferRef, vk::IndexType> /* index buffer */,
                        std::vector<BufferRef> /* vertex buffers */,
                        IndirectCommandsBuffer> indirectDrawCalls;
    std::set<IndirectCommandsBuffer*> indirectCommandBuffers;

    std::unique_ptr<Quad> quad;

	bool _initialized = false;
};


}
