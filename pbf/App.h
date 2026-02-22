//
// Created by daniel on 1/3/25.
//
#pragma once
#include "VulkanContext.h"
#include "Buffer.h"

namespace pbf
{
class Simulation;
class GUIRenderer;

struct GlobalAppData
{
    GlobalAppData(VulkanContext& _context);
    CacheReference<descriptors::DescriptorSetLayout> globalDescriptorSetLayout() const {
        return _globalDescriptorSetLayout;
    };
    const vk::DescriptorSet &globalDescriptorSet() const { return _globalDescriptorSet; }

    static constexpr std::uint32_t numGlobalDescriptorSets = 1;
    CacheReference<descriptors::DescriptorSetLayout> _globalDescriptorSetLayout;
    vk::DescriptorSet _globalDescriptorSet;

};

class App {
public:
    App();
    ~App();

    const Renderer &renderer() const { return *_renderer; }
    Renderer &renderer() { return *_renderer; }
    Scene &scene() { return *_scene; }
    GUI& gui() { return *_gui; }
    Camera& camera() { return *_camera; }

    void run();
private:
    VulkanContext vulkanContext;

    GlobalAppData globalAppData;

    struct GlobalUniformData {
        glm::mat4 mvpmatrix;
        glm::mat4 invviewmat;
        glm::mat4 viewmat;
        glm::mat4 invprojmat;
        glm::mat4 projmat;
        glm::mat3x4 viewrot;
    };
    std::unique_ptr<Buffer<GlobalUniformData>> _globalUniformBuffer;
    GlobalUniformData *globalUniformData = nullptr;

    std::unique_ptr<GUI> _gui;

    std::unique_ptr<Renderer> _renderer;

    std::unique_ptr<Simulation> _simulation;

    std::unique_ptr<Scene> _scene;

    std::unique_ptr<Camera> _camera;
};

}

