//
// Created by daniel on 1/3/25.
//

#include "App.h"
#include "Renderer.h"
#include "GUI.h"
#include "Scene.h"
#include "VulkanContext.h"
#include "SurfaceReconstruction.h"

namespace pbf
{

GlobalAppData::GlobalAppData(VulkanContext& context)
{

    {
        _globalDescriptorSetLayout = context.cache().fetch(descriptors::DescriptorSetLayout{
                .createFlags = {},
                .bindings = {{
                                     .binding = 0,
                                     .descriptorType = vk::DescriptorType::eUniformBuffer,
                                     .descriptorCount = 1,
                                     .stageFlags = vk::ShaderStageFlagBits::eAll
                             }},
            PBF_DESC_DEBUG_NAME("Global Descriptor Set Layout")
        });
        /*
                for(auto [setLayout, descriptor] : crampl::ContainerContainer(_globalDescriptorSetLayouts,
                                                                              setLayoutDescriptors)) {
                    setLayout = descriptor.realize(this);
                }

                std::array<vk::DescriptorSetLayout, numGlobalDescriptorSets> globalDescriptorSetLayouts;
                std::transform(_globalDescriptorSetLayouts.begin(), _globalDescriptorSetLayouts.end(),
                        globalDescriptorSetLayouts.begin(), [](const auto& f) {return *f;});*/
        _globalDescriptorSet = context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
            .descriptorPool = context.descriptorPool(),
            .descriptorSetCount = numGlobalDescriptorSets,
            .pSetLayouts = &*_globalDescriptorSetLayout,
        }).front();

    }
}


App::App(): globalAppData(vulkanContext)
{
    InitContext initContext(vulkanContext);

    initContext.initCommandBuffer->begin(vk::CommandBufferBeginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        .pInheritanceInfo = nullptr
    });

    _renderer = std::make_unique<Renderer>(initContext, globalAppData);
    _gui = std::make_unique<GUI>(initContext, *_renderer, globalAppData);
    _scene = std::make_unique<Scene>(initContext, *_gui, *_renderer, globalAppData);
    _camera = std::make_unique<Camera>(vulkanContext, *_gui);

    initContext.initCommandBuffer->end();


    {
        _globalUniformBuffer = std::make_unique<Buffer<GlobalUniformData>>(vulkanContext, 1, vk::BufferUsageFlagBits::eUniformBuffer, MemoryType::DYNAMIC);
        vk::DescriptorBufferInfo uniformBufferDescriptorInfo {
            _globalUniformBuffer->buffer(), 0, sizeof(GlobalUniformData)
        };
        globalUniformData = _globalUniformBuffer->data();
        vulkanContext.device().updateDescriptorSets({vk::WriteDescriptorSet{
            .dstSet = globalAppData.globalDescriptorSet(),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pImageInfo = nullptr,
            .pBufferInfo = &uniformBufferDescriptorInfo,
            .pTexelBufferView = nullptr
        }}, {});
    }

    vulkanContext.graphicsQueue().submit({
                              vk::SubmitInfo{
                                  .waitSemaphoreCount = 0,
                                  .pWaitSemaphores = nullptr,
                                  .pWaitDstStageMask = {},
                                  .commandBufferCount = 1,
                                  .pCommandBuffers = &*initContext.initCommandBuffer,
                                  .signalSemaphoreCount = 0, // TODO: replace waitIdle below with a semaphore
                                  .pSignalSemaphores = nullptr
                              }
    });
    vulkanContext.graphicsQueue().waitIdle();

    _gui->postInitCleanup();
}

App::~App()
{
}

void App::run()
{
    spdlog::get("console")->debug("Entering main loop.");
    float rot = 0.0f;
    double lastTime = glfwGetTime();
    _camera->SetPosition(glm::vec3 (0, 0, -100));
    while (!vulkanContext.window().shouldClose()) {
        double now = glfwGetTime();
        double timePassed = now - lastTime;
        lastTime = now;
        vulkanContext.pollEvents();
        auto [width, height] = vulkanContext.window().framebufferSize();

        glm::mat4x4 clip = glm::mat4x4( 1.0f,  0.0f, 0.0f, 0.0f,
                                        0.0f, -1.0f, 0.0f, 0.0f,
                                        0.0f,  0.0f, 0.5f, 0.0f,
                                        0.0f,  0.0f, 0.5f, 1.0f);
        glm::mat4 projmat = glm::perspective(glm::radians(60.0f), float(width) / float(height), 0.1f, 1000.0f);
        glm::mat4 mvmat = _camera->GetViewMatrix(); // glm::rotate(glm::translate(glm::mat4(1), glm::vec3(0,0,-3)), rot, glm::vec3(0,0,1));
        rot += 1.0f * timePassed;
        globalUniformData->mvpmatrix = clip * projmat * mvmat;
        globalUniformData->viewrot = _camera->GetViewRot();
        globalUniformData->invviewmat = glm::inverse(mvmat);
        globalUniformData->viewmat = mvmat;
        globalUniformData->invprojmat = glm::inverse(clip * projmat);
        globalUniformData->projmat = clip * projmat;
        globalAppData.globalDescriptorSetLayout().keepAlive();
        _renderer->render(*_scene, *_gui, glm::clamp(timePassed, 1.0 / 1000.0, 1.0 / 20.0));
        vulkanContext.cache().frame();
    }
    spdlog::get("console")->debug("Exiting main loop. Waiting for idle device.");
    vulkanContext.device().waitIdle();
    spdlog::get("console")->debug("Returning from main loop.");

}



}