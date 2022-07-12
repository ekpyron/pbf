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

    void frame();

    void enqueueCommands(vk::CommandBuffer &buf);

    Context* context() {
        return _context;
    }

    IndirectCommandsBuffer* getIndirectCommandBuffer(const CacheReference<descriptors::GraphicsPipeline> &graphicsPipeline,
													 const Quad::PushConstantData& pushConstantData,
                                                     const std::tuple<BufferRef, vk::IndexType> &indexBuffer,
                                                     const std::vector<BufferRef> &vertexBuffers)
    {
        auto result = &crampl::emplace(indirectDrawCalls, std::piecewise_construct, std::forward_as_tuple(graphicsPipeline), std::forward_as_tuple(pushConstantData),
                                       std::forward_as_tuple(indexBuffer), std::forward_as_tuple(vertexBuffers), std::forward_as_tuple(_context));
        indirectCommandBuffers.emplace(result);
        return result;
    }

	Simulation& simulation() { return _simulation; }
	const Simulation& simulation() const { return _simulation; }

private:

    Context* _context;
	Simulation _simulation;

    crampl::MultiKeyMap<std::map,
                        CacheReference<descriptors::GraphicsPipeline>,
						Quad::PushConstantData,
                        std::tuple<BufferRef, vk::IndexType> /* index buffer */,
                        std::vector<BufferRef> /* vertex buffers */,
                        IndirectCommandsBuffer> indirectDrawCalls;
    std::set<IndirectCommandsBuffer*> indirectCommandBuffers;

    std::unique_ptr<Quad> quad;
};


}
