/**
 *
 *
 * @file glfw.h
 * @brief 
 * @author clonker
 * @date 9/26/18
 */
#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <stdexcept>
#include <vector>

namespace pbf {
namespace glfw {
class GLFW {
public:
    GLFW() {
        if (!glfwInit()) {
            throw std::runtime_error("Could not initialize GLFW.");
        }
    }

    ~GLFW() {
        glfwTerminate();
    }

    GLFW(const GLFW &) = delete;

    GLFW &operator=(const GLFW &) = delete;

    auto vulkanSupported() const { return glfwVulkanSupported(); }

    void pollEvents() {
        glfwPollEvents();
    }

    void windowHint(int hint, int value) const {
        glfwWindowHint(hint, value);
    }

    auto getPrimaryMonitor() const { return glfwGetPrimaryMonitor(); }

    auto getRequiredInstanceExtensions() const {
        unsigned int count;
        const char **exts = glfwGetRequiredInstanceExtensions(&count);
        return std::vector<const char *>(exts, exts + count);
    }
};

class Window {
public:
    template<typename...Args>
    Window(Args &&... args) : _window(glfwCreateWindow(std::forward<Args>(args)...)) {
        if (!_window) {
            throw std::runtime_error("Could not create GLFW window");
        }
    }

    ~Window() {
        glfwDestroyWindow(_window);
    }

    Window(const Window &) = delete;

    Window &operator=(const Window &) = delete;

    bool shouldClose() const {
        return static_cast<bool>(glfwWindowShouldClose(_window));
    }

    vk::UniqueSurfaceKHR createSurface(const vk::Instance &instance) const {
        VkSurfaceKHR surface;
        auto result = glfwCreateWindowSurface(instance, _window, nullptr, &surface);
        if(result != VK_SUCCESS) {
            vk::throwResultException(static_cast<vk::Result>(result), "Cannot create Vulkan surface");
        }
        return vk::UniqueSurfaceKHR(surface, {instance});
    }

    auto framebufferSize() const {
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);
        return std::make_tuple(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    }

	GLFWwindow* window() {
		return _window;
	}

private:
    GLFWwindow *_window;


};

}
}
