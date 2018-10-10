/**
 *
 *
 * @file SwapChain.h
 * @brief 
 * @author clonker
 * @date 9/26/18
 */
#pragma once

#include <vulkan/vulkan.hpp>
#include <pbf/Context.h>

namespace pbf {

class Swapchain {
public:
    Swapchain(Context* context, const vk::RenderPass& renderPass, vk::SwapchainKHR oldSwapChain = nullptr);

    const vk::SwapchainKHR &swapchain() const { return *_swapchain; }

    const std::vector<vk::Image> &images() const { return _images; }
    const std::vector<vk::UniqueImageView> &imageViews() const { return _imageViews; };
    const std::vector<vk::UniqueFramebuffer> &frameBuffers() const { return _frameBuffers; };
    const vk::Extent2D &extent() const { return _extent; }

    std::size_t size() const {
        return _images.size();
    }
private:
    vk::Extent2D _extent;
    vk::UniqueSwapchainKHR _swapchain;
    std::vector<vk::Image> _images;
    std::vector<vk::UniqueImageView> _imageViews;
    std::vector<vk::UniqueFramebuffer> _frameBuffers;
};

}
