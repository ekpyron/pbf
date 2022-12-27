#include "UIControlled.h"
#include <pbf/GUI.h>

namespace pbf {

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