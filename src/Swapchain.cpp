/**
 *
 *
 * @file SwapChain.cpp
 * @brief 
 * @author clonker
 * @date 9/26/18
 */
#include "Swapchain.h"
#include "glm/glm.hpp"

namespace pbf {

Swapchain::Swapchain(Context *context, vk::SwapchainKHR oldSwapChain) {
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
        _imageViews.emplace_back(device.createImageViewUnique({
            {}, image, vk::ImageViewType::e2D, context->surfaceFormat().format, {},
            { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
        }));
    }

    _frameBuffers.reserve(_images.size());
    /*for(const auto &iv : _imageViews) {
        _frameBuffers.emplace_back(device.createFramebufferUnique(

                ));
    }*/
}
}
