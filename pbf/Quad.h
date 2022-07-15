//
// Created by daniel on 04.07.22.
//

#ifndef PBF_QUAD_H
#define PBF_QUAD_H

#include <map>
#include <crampl/MultiKeyMap.h>
#include <list>
#include "Context.h"
#include "Buffer.h"
#include "descriptors/GraphicsPipeline.h"


namespace pbf {

class Scene;
class IndirectCommandsBuffer;

class Quad {
public:

    Quad(Scene* scene);

    void frame(uint32_t instanceCount);

private:
    struct VertexData {
        float vertices[4];
    };

    Scene *scene;
    bool active = true;
    bool dirty = true;
    Buffer buffer;
    Buffer indexBuffer;
    IndirectCommandsBuffer* indirectCommandsBuffer {nullptr};
	CacheReference<descriptors::GraphicsPipeline> graphicsPipeline;
};

}


#endif //PBF_QUAD_H
