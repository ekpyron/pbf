#include "Camera.h"
#include "Context.h"
#include <imgui.h>

namespace pbf {

Camera::Camera(Context& context): UIControlled(context.gui()), context(context)
{
}

void Camera::ui()
{
	bool doNavigation = !ImGui::IsAnyItemActive() && !ImGui::GetIO().WantCaptureMouse;

	if (doNavigation)
	{
		ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
		ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
		auto [width, height] = context.window().framebufferSize();
		dragDelta.x /= float(width);
		dragDelta.y /= float(height);

		if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
		{
			context.camera().Zoom(dragDelta.x + dragDelta.y);
		}
		else if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
		{
			context.camera().MoveX (dragDelta.x);
			context.camera().MoveY (dragDelta.y);
		}
		else
		{
			context.camera().Rotate (dragDelta.y, -dragDelta.x);
		}
	}
}

const glm::vec3 &Camera::GetPosition () const
{
    return pos;
}

glm::mat4 Camera::GetViewMatrix () const
{
    glm::vec3 dir = rot * glm::vec3 (0, 0, 1);
    glm::vec3 up = rot * glm::vec3 (0, 1, 0);
    glm::mat4 vmat = glm::lookAt (pos, pos + dir, up);
    return vmat;
}

glm::mat4 Camera::GetViewRot() const {
	glm::vec3 right = rot * glm::vec3(1,0,0);
	glm::vec3 up = rot * glm::vec3(0,1,0);
	glm::vec3 depth = glm::cross(right, up);
	return glm::mat4(glm::mat3(right, up, depth));
}


void Camera::SetPosition (const glm::vec3 &_pos)
{
    pos = _pos;
}

void Camera::Rotate (float _xangle, float _yangle)
{
    xangle += 2.0f * _xangle;
    yangle += 2.0f * _yangle;

    rot = glm::rotate (glm::quat (1, 0, 0, 0), yangle, glm::vec3 (0.0f, 1.0f, 0.0f));
    rot = glm::rotate (rot, xangle, glm::vec3 (1.0f, 0.0f, 0.0f));
}

void Camera::Zoom (float value)
{
    pos += 50.0f * value * (rot * glm::vec3 (0, 0, 1));
}

void Camera::MoveX (float value)
{
    pos += 50.0f * value * (rot * glm::vec3 (1, 0, 0));
}

void Camera::MoveY (float value)
{
    pos += 50.0f * value * (rot * glm::vec3 (0, 1, 0));
}

}
