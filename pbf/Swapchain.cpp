/**
 *
 *
 * @file SwapChain.cpp
 * @brief 
 * @author clonker
 * @date 9/26/18
 */
#include "Swapchain.h"
#include <glm/glm.hpp>
#include <pbf/Renderer.h>

namespace pbf {

Swapchain::Swapchain(Context *context, const vk::RenderPass& renderPass, vk::SwapchainKHR oldSwapChain) {
    const auto &device = context->device();
    const auto &physicalDevice = context->physicalDevice();
    const auto &surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(context->surface());

    {
        if (surfaceCapabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
            _extent = surfaceCapabilities.currentExtent;
        } else {
            auto[width, height] = context->window().framebufferSize();
            _extent = vk::Extent2D{
                    glm::clamp(width, surfaceCapabilities.minImageExtent.width,
                               surfaceCapabilities.maxImageExtent.width),
                    glm::clamp(height, surfaceCapabilities.minImageExtent.height,
                               surfaceCapabilities.maxImageExtent.height)
            };
        }
    }

    {
        std::size_t nOffscreen = 1;
        auto minImageCount = static_cast<uint32_t>(surfaceCapabilities.minImageCount + nOffscreen);
        vk::SwapchainCreateInfoKHR createInfo;
        createInfo.setSurface(context->surface())
                .setMinImageCount(minImageCount)
                .setImageFormat(context->surfaceFormat().format)
                .setImageColorSpace(context->surfaceFormat().colorSpace)
                .setImageExtent(_extent)
                .setImageArrayLayers(1)
                .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                .setPreTransform(surfaceCapabilities.currentTransform)
                .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                .setPresentMode(context->presentMode())
                .setClipped(VK_TRUE)
                .setOldSwapchain(oldSwapChain);

        if(context->families().same()) {
            createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        } else {
            createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(context->families().arr.size())
                .setPQueueFamilyIndices(context->families().arr.data());
        }
        _swapchain = device.createSwapchainKHRUnique(createInfo);
    }

    _images = device.getSwapchainImagesKHR(*_swapchain);
    _imageViews.reserve(_images.size());
    for(const auto &image : _images) {
        _imageViews.emplace_back(device.createImageViewUnique(vk::ImageViewCreateInfo{
			.image = image,
			.viewType = vk::ImageViewType::e2D,
			.format = context->surfaceFormat().format,
			.subresourceRange = vk::ImageSubresourceRange{
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		}));
    }

    _frameBuffers.reserve(_images.size());
    for(const auto &iv : _imageViews) {
        _frameBuffers.emplace_back(device.createFramebufferUnique(
                vk::FramebufferCreateInfo{
					.renderPass = renderPass,
					.attachmentCount = 1,
					.pAttachments = &*iv,
					.width = _extent.width,
					.height = _extent.height,
					.layers = 1,
				}
        ));
    }
#ifndef NDEBUG
    ++swapchainIncarnation;
    PBF_DEBUG_SET_OBJECT_NAME(context, *_swapchain, fmt::format("Swapchain <{}>", swapchainIncarnation));
    for (std::size_t i = 0; i < _images.size(); i++) {
        PBF_DEBUG_SET_OBJECT_NAME(context, _images[i], fmt::format("Swapchain <{}> Image #{}", swapchainIncarnation, i));
        PBF_DEBUG_SET_OBJECT_NAME(context, *_imageViews[i], fmt::format("Swapchain <{}> Image View #{}", swapchainIncarnation, i));
        PBF_DEBUG_SET_OBJECT_NAME(context, *_frameBuffers[i], fmt::format("Swapchain <{}> Framebuffer #{}", swapchainIncarnation, i));
    }
#endif
}

#ifndef NDEBUG
std::uint64_t Swapchain::swapchainIncarnation = 0;
#endif

}
