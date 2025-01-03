//
// Created by daniel on 04.07.22.
//

#ifndef PBF_QUAD_H
#define PBF_QUAD_H

#include <map>
#include <crampl/MultiKeyMap.h>
#include <list>
#include "VulkanContext.h"
#include "Buffer.h"
#include "descriptors/GraphicsPipeline.h"


namespace pbf {

class Scene;
class IndirectCommandsBuffer;
struct GlobalAppData;

class Quad {
public:
    Quad(InitContext& initContext, Scene& scene, Renderer& renderer, GlobalAppData& _data);

    void frame(uint32_t instanceCount);

    struct VertexData {
        float vertices[4];
    };
private:
    Scene& scene;
    Renderer& renderer;
    bool active = true;
    Buffer<VertexData> buffer;
    Buffer<std::uint16_t> indexBuffer;
    IndirectCommandsBuffer* indirectCommandsBuffer {nullptr};
	CacheReference<descriptors::GraphicsPipeline> graphicsPipeline;
};

}


#endif //PBF_QUAD_H
