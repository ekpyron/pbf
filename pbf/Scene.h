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

class Scene {
public:

    Scene(InitContext& context);

    void frame(vk::CommandBuffer &buf);

    void enqueueCommands(vk::CommandBuffer &buf);

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

	auto& simulations() { return _simulations; }
	[[nodiscard]] const auto& simulations() const { return _simulations; }
	[[nodiscard]] uint32_t getNumParticles() const {
		return _numParticles;
	}

    struct ParticleData {
        glm::vec3 position;
        float aux = 0.0f;
    };

	Buffer<ParticleData>& particleData() { return _particleData; }
	[[nodiscard]] const Buffer<ParticleData>& particleData() const { return _particleData; }

private:

    Context& _context;

	uint32_t _numParticles = 64 * 64 * 64;

	Buffer<ParticleData> _particleData;

	std::vector<Simulation> _simulations;

	crampl::MultiKeyMap<std::map,
                        CacheReference<descriptors::GraphicsPipeline>,
						std::vector<uint8_t>,
                        std::tuple<BufferRef<std::uint16_t>, vk::IndexType> /* index buffer */,
                        std::vector<BufferRef<Quad::VertexData>> /* vertex buffers */,
                        IndirectCommandsBuffer> indirectDrawCalls;
    std::set<IndirectCommandsBuffer*> indirectCommandBuffers;

    Quad quad;
};


}
