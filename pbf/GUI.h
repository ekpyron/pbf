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
	GUI(InitContext& _initContext);
	GUI(const GUI&) = delete;
	GUI& operator=(const GUI&) = delete;
	~GUI();
	void postInitCleanup();
	void render(vk::CommandBuffer buf);
	bool runSPH() const { return _runSPH; }

private:
	friend class UIControlled;
	void add(UIControlled* uiControlled) {
		_uiControlled.emplace_back(uiControlled);
	}
	void remove(UIControlled* uiControlled) {
		std::erase(_uiControlled, uiControlled);
	}

	std::vector<UIControlled*> _uiControlled;
	Context& _context;
	Selection _selection;
	bool _runSPH = false;
	std::mutex _imguiMutex;
	ImGuiContext* _imguiContext = nullptr;
};

}
