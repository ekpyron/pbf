#pragma once

#include <pbf/common.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <thread>
#include <pbf/Selection.h>

namespace pbf {

class UIControlled;

class GUI
{
public:
	GUI() = default;
	GUI(const GUI&) = delete;
	GUI& operator=(const GUI&) = delete;
	~GUI() = default;

private:
	friend class UIControlled;
	friend class GUIRenderer;
	void add(UIControlled* uiControlled, bool _front = false) {
		if (_front)
			_uiControlled.emplace(_uiControlled.begin(), uiControlled);
		else
			_uiControlled.emplace_back(uiControlled);
	}
	void remove(UIControlled* uiControlled) {
		std::erase(_uiControlled, uiControlled);
	}

	std::vector<UIControlled*> _uiControlled;
};

class GUIRenderer {
public:
	GUIRenderer(InitContext& _initContext, GUI& gui, Renderer& renderer, GlobalAppData& globalAppData);
	GUIRenderer(const GUIRenderer&) = delete;
	GUIRenderer& operator=(const GUIRenderer&) = delete;
	~GUIRenderer();
	void render(Scene& scene, vk::CommandBuffer buf);

	VulkanContext& _context;
	GUI& _gui;
	Selection _selection;
	std::mutex _imguiMutex;
	ImGuiContext* _imguiContext = nullptr;
};

}
