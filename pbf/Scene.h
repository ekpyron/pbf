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

namespace pbf {
class Scene {
public:

    Scene(Context* context);

    void enqueueCommands(vk::CommandBuffer &buf);

private:
    Context* context;
};
}

