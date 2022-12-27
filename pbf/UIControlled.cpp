#include "UIControlled.h"
#include <pbf/GUI.h>

namespace pbf {

UIControlled::UIControlled(GUI &gui): _gui(&gui)
{
	addMyself();
}

UIControlled::~UIControlled()
{
	removeMyself();
}

void UIControlled::removeMyself()
{
	if (_gui)
	{
		_gui->remove(this);
		_gui = nullptr;
	}
}

void UIControlled::addMyself()
{
	if (_gui)
		_gui->add(this);
}

}