#pragma once

#include <utility>

namespace pbf {

class GUI;

// Note: any UIControlled object must have shorter lifetime than the GUI.
class UIControlled {
public:
	UIControlled(GUI& gui);
	UIControlled(UIControlled&& _uic) { (*this) = std::move(_uic); }
	UIControlled(UIControlled const&) = delete;
	UIControlled& operator=(UIControlled const&) = delete;
	UIControlled& operator=(UIControlled&& _uic)
	{
		_gui = _uic._gui;
		_uic.removeMyself();
		addMyself();
		return *this;
	}
	virtual ~UIControlled();
protected:
	virtual void ui() = 0;
private:
	void addMyself();
	void removeMyself();
	GUI* _gui = nullptr;
	friend class GUI;
};

}
