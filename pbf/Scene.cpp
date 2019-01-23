/**
 *
 *
 * @file Scene.cpp
 * @brief 
 * @author clonker
 * @date 1/23/19
 */
#include "Scene.h"

namespace pbf {

Scene::Scene(Context *context) : _context(context) {

}

void Scene::enqueueCommands(vk::CommandBuffer &buf) {
    const auto &device = _context->device();


}

}
