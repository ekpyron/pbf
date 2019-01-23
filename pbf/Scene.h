/**
 *
 *
 * @file Scene.h
 * @brief 
 * @author clonker
 * @date 1/23/19
 */
#pragma once

#include "Context.h"
#include "Buffer.h"

namespace pbf {
class Scene {
public:

    Scene(Context* context);

    void enqueueCommands(vk::CommandBuffer &buf);

    Context* context() {
        return _context;
    }

private:
    Context* _context;
};

class Triangle {
public:

    Triangle(Scene* scene) :scene(scene) {

        struct Data{
            uint16_t indices[4];
            float vertices[4*4];
        };

        buffer = {scene->context(), sizeof(Data),
                  vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer
                  | vk::BufferUsageFlagBits::eIndexBuffer, MemoryType::STATIC};
        initializeBuffer = {scene->context(), sizeof(Data), vk::BufferUsageFlagBits::eTransferSrc, MemoryType::TRANSIENT};

        Data *data = reinterpret_cast<Data*>(initializeBuffer.data());
        data->indices[0] = 0;
        data->indices[1] = 1;
        data->indices[2] = 2;
        data->vertices[0] = 0.0f;
        data->vertices[1] = 1.0f;
        data->vertices[2] = 0.0f;
        data->vertices[4] = -1.0f;
        data->vertices[5] = -1.0f;
        data->vertices[6] = 0.0f;
        data->vertices[8] = 1.0f;
        data->vertices[9] = -1.0f;
        data->vertices[10] = 0.0f;
        initializeBuffer.flush();


    }

    void frame(vk::CommandBuffer& enqueueBuffer) {
        if (!isInitialized) {
            auto const& device = scene->context()->device();

            enqueueBuffer.copyBuffer(initializeBuffer.buffer(), buffer.buffer(), {
                vk::BufferCopy {
                    0, 0, buffer.size()
                }
            });

            // copy

        }
        if (active) {
            if (dirty) {
                enqueueBuffer.waitEvents(
                        {}, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexInput, {}, {vk::BufferMemoryBarrier{
                            vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eInputAttachmentRead, 0, 0, buffer.buffer(), 0, buffer.size()
                        }}, {}
                        );
                dirty = false;
            }
            // if (potentiallyVisisble() && occlusionQuery()) {
            // draw
            // }
        }
    }

private:
    Scene *scene;
    bool isInitialized = false;
    bool active = true;
    bool dirty = true;
    Buffer initializeBuffer;
    Buffer buffer;
};


}
