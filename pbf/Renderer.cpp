/**
 *
 *
 * @file Renderer.cpp
 * @brief 
 * @author clonker
 * @date 9/26/18
 */
#include <pbf/descriptors/GraphicsPipeline.h>
#include "Renderer.h"
#include "Scene.h"

static constexpr std::uint64_t TIMEOUT = std::numeric_limits<std::uint64_t>::max();

namespace pbf {

Renderer::Renderer(Context *context) : _context(context) {
    constexpr std::size_t FRAME_PRERENDER_COUNT = 3; // todo: choose this wisely
    _frameSync.reserve(FRAME_PRERENDER_COUNT);
    for (size_t i = 0; i < FRAME_PRERENDER_COUNT; i++) {
        _frameSync.emplace_back(FrameSync{
                context->device().createSemaphoreUnique({}),
                context->device().createSemaphoreUnique({}),
                context->device().createFenceUnique({vk::FenceCreateFlagBits::eSignaled})
        });
        PBF_DEBUG_SET_OBJECT_NAME(context, *_frameSync.back().imageAvailableSemaphore,
                                  fmt::format("Image Available Semaphore #{}", i));
        PBF_DEBUG_SET_OBJECT_NAME(context, *_frameSync.back().renderFinishedSemaphore,
                                  fmt::format("Render Finished Semaphore #{}", i));
        PBF_DEBUG_SET_OBJECT_NAME(context, *_frameSync.back().fence, fmt::format("Frame Fence #{}", i));
    }

    {
        _renderPass = context->cache().fetch(descriptors::RenderPass{
                .attachments = {
                        vk::AttachmentDescription{
                                {},
                                _context->surfaceFormat().format,
                                vk::SampleCountFlagBits::e1,
                                vk::AttachmentLoadOp::eClear,
                                vk::AttachmentStoreOp::eStore,
                                vk::AttachmentLoadOp::eDontCare,
                                vk::AttachmentStoreOp::eDontCare,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::ePresentSrcKHR
                        }
                },
                .subpasses = {
                        {
                                .flags = {},
                                .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
                                .inputAttachments = {},
                                .colorAttachments = {{0, vk::ImageLayout::eColorAttachmentOptimal}},
                                .resolveAttachments = {},
                                .depthStencilAttachment = {},
                                .preserveAttachments = {}
                        }
                },
                .subpassDependencies = {
                        vk::SubpassDependency{
                                VK_SUBPASS_EXTERNAL,
                                0,
                                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                {}, {}, {}
                        }
                },
                PBF_DESC_DEBUG_NAME("Main Renderer RenderPass")
        });
    }
    {
        _graphicsPipeline = context->cache().fetch(descriptors::GraphicsPipeline{
                .shaderStages = {
                        {
                                .stage = vk::ShaderStageFlagBits::eVertex,
                                .module = context->cache().fetch(descriptors::ShaderModule{
                                        .filename = "shaders/test.vert.spv",
                                        PBF_DESC_DEBUG_NAME("shaders/test.vert.spv Vertex Shader")
                                }),
                                .entryPoint = "main"
                        },
                        {
                                .stage = vk::ShaderStageFlagBits::eFragment,
                                .module = context->cache().fetch(descriptors::ShaderModule{
                                        .filename = "shaders/test.frag.spv",
                                        PBF_DESC_DEBUG_NAME("Fragment Shader shaders/test.frag.spv")
                                }),
                                .entryPoint = "main"
                        }
                },
                .vertexBindingDescriptions = {},
                .vertexInputAttributeDescriptions = {},
                .primitiveTopology = vk::PrimitiveTopology::eTriangleList,
                .rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo().setLineWidth(1.0f),
                .colorBlendAttachmentStates = {
                        vk::PipelineColorBlendAttachmentState().setColorWriteMask(
                                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
                        )
                },
                .dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
                .pipelineLayout = context->cache().fetch(descriptors::PipelineLayout{
                        PBF_DESC_DEBUG_NAME("Dummy Pipeline Layout")
                }),
                .renderPass = _renderPass,
                PBF_DESC_DEBUG_NAME("Main Renderer Graphics Pipeline")
        });
    }
    reset();
}

void Renderer::render() {
    const auto &device = _context->device();

    auto &currentFrameSync = _frameSync[_currentFrameSync];

    device.waitForFences({*currentFrameSync.fence}, static_cast<vk::Bool32>(true), TIMEOUT);
    currentFrameSync.commandBuffer.reset();

    static auto lastTime = std::chrono::steady_clock::now();
    static size_t frameCount = 0;

    frameCount++;
    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastTime).count() >= 1000) {
        lastTime = std::chrono::steady_clock::now();
        spdlog::get("console")->debug("FPS {}", frameCount);
        frameCount = 0;
    }

//    _renderPass.keepAlive(); --> is kept alive by graphics pipeline (probably)
    _graphicsPipeline.keepAlive();

    uint32_t imageIndex = 0;
    auto result = device.acquireNextImageKHR(
            _swapchain->swapchain(), TIMEOUT, *currentFrameSync.imageAvailableSemaphore,
            nullptr, &imageIndex
    );

    switch (result) {
        case vk::Result::eErrorOutOfDateKHR: {
            reset();
            return;
        }
        case vk::Result::eSuccess:
        case vk::Result::eSuboptimalKHR:
            break;
        default: {
            vk::throwResultException(result, "cannot acquire image");
        }
    }

    auto buffer = std::move(device.allocateCommandBuffersUnique({
        _context->commandPool(true), vk::CommandBufferLevel::ePrimary, 1U
    }).front());
    {

#ifndef NDEBUG
        static std::size_t __counter{0};
        PBF_DEBUG_SET_OBJECT_NAME(_context, *buffer, fmt::format("Scene Command Buffer #{}", __counter++));
#endif

        buffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr});

        _context->scene().frame(*buffer);

        vk::ClearValue clearValue;
        clearValue.setColor({std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f}});
        buffer->setViewport(0, {
            vk::Viewport{
                0, 0, float(_swapchain->extent().width), float(_swapchain->extent().height), 0.0f, 1.0f
            }
        });
        buffer->setScissor(0, {vk::Rect2D{vk::Offset2D(), _swapchain->extent()}});
        buffer->beginRenderPass(vk::RenderPassBeginInfo{
                *_renderPass, *_swapchain->frameBuffers()[imageIndex],
                vk::Rect2D{{}, _swapchain->extent()}, 1, &clearValue
        }, vk::SubpassContents::eInline);
        /*buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *_graphicsPipeline);
        buffer->draw(3, 1, 0, 0);*/

        _context->scene().enqueueCommands(*buffer);

        buffer->endRenderPass();
        buffer->end();
    }


    device.resetFences({*currentFrameSync.fence});

    vk::PipelineStageFlags waitStages[] = {
            vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    _context->graphicsQueue().submit({vk::SubmitInfo{
            1, &*currentFrameSync.imageAvailableSemaphore, waitStages,
            1, &*buffer, 1, &*currentFrameSync.renderFinishedSemaphore
    }}, *currentFrameSync.fence);

    currentFrameSync.commandBuffer = std::move(buffer);

    _context->presentQueue().presentKHR({
        1, &*currentFrameSync.renderFinishedSemaphore, 1,
        &_swapchain->swapchain(), &imageIndex, nullptr
    });

    _currentFrameSync = (_currentFrameSync + 1) % _frameSync.size();
}

void Renderer::reset() {
    auto const &device = _context->device();
    if (_swapchain)
        device.waitIdle();
    _swapchain = std::make_unique<Swapchain>(_context, *_renderPass, _swapchain ? _swapchain->swapchain() : nullptr);
}

}
