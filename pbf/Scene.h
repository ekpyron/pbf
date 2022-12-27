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
#include <list>

#include <crampl/MultiKeyMap.h>

#include "Context.h"
#include "Buffer.h"
#include "Quad.h"
#include "IndirectCommandsBuffer.h"
#include "Simulation.h"

namespace pbf {

struct ParticleData {
	glm::vec3 position;
	uint32_t aux = 0;
	glm::vec3 velocity;
	uint32_t type = 0;
};

struct ParticleKey {
	glm::vec3 position;
	uint32_t idx = 0;
};

class Scene {
public:

    Scene(InitContext& context);

    void frame(vk::CommandBuffer &buf);

    void enqueueCommands(vk::CommandBuffer &buf);

	void resetParticles();

    Context& context() {
        return _context;
    }

    IndirectCommandsBuffer* getIndirectCommandBuffer(const CacheReference<descriptors::GraphicsPipeline> &graphicsPipeline,
													 const std::vector<uint8_t>& pushConstantData,
                                                     const std::tuple<BufferRef<std::uint16_t>, vk::IndexType> &indexBuffer,
                                                     const std::vector<BufferRef<Quad::VertexData>> &vertexBuffers)
    {
        auto result = &crampl::emplace(indirectDrawCalls, std::piecewise_construct, std::forward_as_tuple(graphicsPipeline), std::forward_as_tuple(pushConstantData),
                                       std::forward_as_tuple(indexBuffer), std::forward_as_tuple(vertexBuffers), std::forward_as_tuple(_context));
        indirectCommandBuffers.emplace(result);
        return result;
    }

	[[nodiscard]] auto& simulation() { return _simulation; }
	[[nodiscard]] const auto& simulation() const { return _simulation; }
	[[nodiscard]] uint32_t getNumParticles() const {
		return _numParticles;
	}

	RingBuffer<ParticleData>& particleData() { return _particleData; }
	[[nodiscard]] const RingBuffer<ParticleData>& particleData() const { return _particleData; }

private:
	bool _resetParticles = false;

    Context& _context;

	uint32_t _numParticles = 64 * 64 * 32;

	RingBuffer<ParticleData> _particleData;

	crampl::MultiKeyMap<std::map,
                        CacheReference<descriptors::GraphicsPipeline>,
						std::vector<uint8_t>,
                        std::tuple<BufferRef<std::uint16_t>, vk::IndexType> /* index buffer */,
                        std::vector<BufferRef<Quad::VertexData>> /* vertex buffers */,
                        IndirectCommandsBuffer> indirectDrawCalls;
    std::set<IndirectCommandsBuffer*> indirectCommandBuffers;

    Quad quad;
	Simulation _simulation;
};


}
