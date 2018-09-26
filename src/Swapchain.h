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
#include "Context.h"

namespace pbf {

class Swapchain {
public:
    Swapchain(Context* context, vk::SwapchainKHR oldSwapChain = nullptr);

private:
    vk::Extent2D _extent;
    vk::UniqueSwapchainKHR _swapchain;
    std::vector<vk::Image> _images;
    std::vector<vk::UniqueImageView> _imageViews;
};

}
