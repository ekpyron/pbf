/**
 *
 *
 * @file Renderer.cpp
 * @brief 
 * @author clonker
 * @date 9/26/18
 */
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
    *_renderPass;
    reset();
}

void Renderer::render() {
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
    _swapchain = std::make_unique<Swapchain>(_context, _swapchain ? _swapchain->swapchain() : nullptr);
    const auto &images = _swapchain->images();
    const auto &imageViews = _swapchain->imageViews();
    _commandBuffers = _context->device().allocateCommandBuffersUnique({
                                                                              _context->commandPool(),
                                                                              vk::CommandBufferLevel::ePrimary,
                                                                              static_cast<uint32_t>(imageViews.size())
                                                                      });

    for (std::size_t i = 0; i < images.size(); ++i) {
        auto &buf = _commandBuffers[i];
        buf->begin({vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr});
        {
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
        }
        buf->end();
    }
}

}
