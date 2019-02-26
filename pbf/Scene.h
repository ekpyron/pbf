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
#include <crampl/MultiKeyMap.h>
#include <list>

namespace pbf {

class Renderable {
public:

protected:
    CacheReference<descriptors::GraphicsPipeline> _graphicsPipeline;

};


class Instanceable {
public:
    void addInstance() {
        ++_instanceCount;
    }
    void removeInstance() {
        --_instanceCount;
    }

    std::uint32_t instanceCount() const {return _instanceCount;}

private:
    std::uint32_t _instanceCount = 0;
};

class Instance {
public:
    Instance(Instanceable* parent = nullptr) : parent(parent) {
        if (parent)
            parent->addInstance();
    }
    ~Instance() {
        if (parent)
            parent->removeInstance();
    }
    Instance(const Instance&) = delete;
    Instance  &operator=(const Instance&) = delete;
    Instance(Instance &&rhs) noexcept {
        *this = std::move(rhs);
    }
    Instance &operator=(Instance &&rhs) noexcept {
        if (parent)
            parent->removeInstance();
        parent = rhs.parent;
        rhs.parent = nullptr;
        return *this;
    }
private:
    Instanceable *parent = nullptr;
};

class Triangle;

class Scene {
public:

    class IndirectCommandsBuffer
    {
    public:
        IndirectCommandsBuffer(Context *context);
        void clear();
        void push_back(const vk::DrawIndirectCommand &cmd);
        const std::list<Buffer> &buffers() const { return _buffers; }
        std::uint32_t elementsInLastBuffer() const { return _elementsInLastBuffer; }
        static constexpr std::uint32_t bufferSize = 128;
    private:
        Context *context;
        std::list<Buffer> _buffers;
        std::list<Buffer>::iterator _currentBuffer;
        std::uint32_t _elementsInLastBuffer {bufferSize};
    };

    Scene(Context* context);

    void frame(vk::CommandBuffer& enqueueBuffer);

    void enqueueCommands(vk::CommandBuffer &buf);

    Context* context() {
        return _context;
    }

    IndirectCommandsBuffer* getIndirectCommandBuffer(const CacheReference<descriptors::GraphicsPipeline> &graphicsPipeline,
                                                     const std::tuple<BufferRef, vk::IndexType> &indexBuffer,
                                                     const std::vector<BufferRef> &vertexBuffers)
    {
        auto result = &crampl::emplace(indirectDrawCalls, std::piecewise_construct, std::forward_as_tuple(graphicsPipeline),
                                       std::forward_as_tuple(indexBuffer), std::forward_as_tuple(vertexBuffers), std::forward_as_tuple(_context));
        indirectCommandBuffers.emplace(result);
        return result;
    }

private:

    Context* _context;

    crampl::MultiKeyMap<std::map,
                        CacheReference<descriptors::GraphicsPipeline>,
                        std::tuple<BufferRef, vk::IndexType> /* index buffer */,
                        std::vector<BufferRef> /* vertex buffers */,
                        IndirectCommandsBuffer> indirectDrawCalls;
    std::set<IndirectCommandsBuffer*> indirectCommandBuffers;

    std::unique_ptr<Triangle> triangle;
    std::unique_ptr<Instance> triangleInstance;
};


class Triangle: public Instanceable {
public:

    Triangle(Scene* scene);

    void frame(vk::CommandBuffer& enqueueBuffer);

private:
    struct VertexData {
        float vertices[4];
    };

    Scene *scene;
    bool isInitialized = false;
    bool active = true;
    bool dirty = true;
    Buffer initializeBuffer;
    vk::UniqueEvent initializedEvent;
    Buffer buffer;
    Buffer indexBuffer;
    Scene::IndirectCommandsBuffer* indirectCommandsBuffer {nullptr};
};


}
