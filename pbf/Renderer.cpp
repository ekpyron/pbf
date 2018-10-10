/**
 *
 *
 * @file Renderer.cpp
 * @brief 
 * @author clonker
 * @date 9/26/18
 */
#include <pbf/objects/GraphicsPipeline.h>
#include "Renderer.h"

static constexpr std::uint64_t TIMEOUT = std::numeric_limits<std::uint64_t>::max();

namespace pbf {

Renderer::Renderer(Context *context) : _context(context) {

    _imageAvailableSemaphore = context->device().createSemaphoreUnique({});
    _renderFinishedSemaphore = context->device().createSemaphoreUnique({});

    {
        auto descriptor = objects::RenderPass::Descriptor{{}};
        descriptor.addAttachment(
                vk::AttachmentDescription{{}, _context->surfaceFormat().format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
                                          vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
                                          vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
                                          vk::ImageLayout::ePresentSrcKHR});
        descriptor.addSubpass(objects::SubpassDescriptor{
                .flags = {},
                .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
                .inputAttachments = {},
                .colorAttachments = {{0, vk::ImageLayout::eColorAttachmentOptimal}},
                .resolveAttachments = {},
                .depthStencilAttachment = {},
                .preserveAttachments = {}
        });
        _renderPass = context->cache().fetch(std::move(descriptor));
    }
    {
        auto descriptor = objects::GraphicsPipeline::Descriptor{};

        descriptor.addDynamicState(vk::DynamicState::eViewport);
        descriptor.addDynamicState(vk::DynamicState::eScissor);
        descriptor.setTopology(vk::PrimitiveTopology::eTriangleList);

        descriptor.addColorBlendAttachmentState(vk::PipelineColorBlendAttachmentState().setColorWriteMask(
            vk::ColorComponentFlagBits::eR|vk::ColorComponentFlagBits::eG|vk::ColorComponentFlagBits::eB|vk::ColorComponentFlagBits::eA
        ));
        descriptor.setRenderPass(_renderPass);
        descriptor.rasterizationStateCreateInfo().setLineWidth(1.0f);
        descriptor.setPipelineLayout(context->cache().fetch(objects::PipelineLayout::Descriptor {}));
        auto shaderModule = context->cache().fetch(objects::ShaderModule::Descriptor {.filename = "shaders/test.spv"});
        descriptor.addShaderStage(vk::ShaderStageFlagBits::eVertex, shaderModule, "main");
        auto fragShaderModule = context->cache().fetch(objects::ShaderModule::Descriptor {.filename = "shaders/test_frag.spv"});
        descriptor.addShaderStage(vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main");

        _graphicsPipeline = context->cache().fetch(std::move(descriptor));
    }
    reset();
}

void Renderer::render() {
    static auto lastTime = std::chrono::steady_clock::now();
    static size_t frameCount = 0;

    frameCount++;
    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastTime).count() >= 1000) {
        lastTime = std::chrono::steady_clock::now();
        spdlog::get("console")->error("Frame count {}", frameCount);
        frameCount = 0;
    }

    _renderPass.keepAlive();
    _graphicsPipeline.keepAlive();

    const auto &device = _context->device();
    auto[result, imageIndex] = device.acquireNextImageKHR(_swapchain->swapchain(), TIMEOUT, *_imageAvailableSemaphore,
                                                          nullptr);

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

    vk::PipelineStageFlags waitStages[] = {
            vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    _context->graphicsQueue().submit({vk::SubmitInfo{
            1, &*_imageAvailableSemaphore, waitStages, 1, &*_commandBuffers[imageIndex], 1, &*_renderFinishedSemaphore
    }}, {});

    _context->presentQueue().presentKHR({1, &*_renderFinishedSemaphore, 1, &_swapchain->swapchain(), &imageIndex,
                                         nullptr});
    _context->presentQueue().waitIdle();
}

void Renderer::reset() {
    _swapchain = std::make_unique<Swapchain>(_context, _renderPass->get(), _swapchain ? _swapchain->swapchain() : nullptr);
    const auto &images = _swapchain->images();
    const auto &imageViews = _swapchain->imageViews();
    const auto &framebuffers = _swapchain->frameBuffers();
    _commandBuffers = _context->device().allocateCommandBuffersUnique({
                                                                              _context->commandPool(),
                                                                              vk::CommandBufferLevel::ePrimary,
                                                                              static_cast<uint32_t>(imageViews.size())
                                                                      });

    for (std::size_t i = 0; i < images.size(); ++i) {
        auto &buf = _commandBuffers[i];
        buf->begin({vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr});
        vk::ClearValue clearValue;
        clearValue.setColor({ std::array<float, 4> { 0.0f, 1.0f, 0.0f, 1.0f }});
        buf->setViewport(0, {vk::Viewport{0, 0, float(_swapchain->extent().width), float(_swapchain->extent().height), 0.0f, 1.0f}});
        buf->setScissor(0, {vk::Rect2D{vk::Offset2D(), _swapchain->extent()}});
        buf->beginRenderPass(vk::RenderPassBeginInfo{
                                     (*_renderPass).get(), *framebuffers[i], vk::Rect2D{{}, _swapchain->extent()}, 1, &clearValue
        }, vk::SubpassContents::eInline);
        buf->bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipeline->get());
        buf->draw(3, 1, 0, 0);
        buf->endRenderPass();
        /*{
            auto imb = vk::ImageMemoryBarrier().setOldLayout(vk::ImageLayout::ePresentSrcKHR)
                    .setNewLayout(vk::ImageLayout::eGeneral)
                    .setSrcQueueFamilyIndex(_context->families().present)
                    .setDstQueueFamilyIndex(_context->families().graphics)
                    .setImage(images[i])
                    .setSubresourceRange(vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
            buf->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, {}, {},
                                 {}, {{imb}});
        }
        buf->clearColorImage(images[i], vk::ImageLayout::eGeneral,
                             vk::ClearColorValue(std::array<float, 4>{{1.f, 0.f, 0.f, 1.f}}),
                             {{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
        {
            auto imb = vk::ImageMemoryBarrier().setOldLayout(vk::ImageLayout::eGeneral)
                    .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
                    .setSrcQueueFamilyIndex(_context->families().graphics)
                    .setDstQueueFamilyIndex(_context->families().present)
                    .setImage(images[i])
                    .setSubresourceRange(vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
            buf->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, {}, {},
                                 {}, {{imb}});
        }*/
        buf->end();
    }
}

}
