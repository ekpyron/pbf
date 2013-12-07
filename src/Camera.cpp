#include "Camera.h"

Camera::Camera (void) : xangle (0.0f), yangle (0.0f)
{
}

Camera::~Camera (void)
{
}

const glm::vec3 &Camera::GetPosition (void) const
{
    return pos;
}

glm::mat4 Camera::GetViewMatrix (void) const
{
    glm::vec3 dir = rot * glm::vec3 (0, 0, 1);
    glm::vec3 up = rot * glm::vec3 (0, 1, 0);
    glm::mat4 vmat = glm::lookAt (pos, pos + dir, up);
    return vmat;
}

void Camera::SetPosition (const glm::vec3 &_pos)
{
    pos = _pos;
}

void Camera::Rotate (const float &_xangle, const float &_yangle)
{
    xangle += _xangle;
    yangle += _yangle;

    rot = glm::rotate (glm::quat (), yangle, glm::vec3 (0.0f, 1.0f, 0.0f));
    rot = glm::rotate (rot, xangle, glm::vec3 (1.0f, 0.0f, 0.0f));
}

void Camera::Zoom (const float &value)
{
    pos += 0.1f * value * (rot * glm::vec3 (0, 0, 1));
}

void Camera::MoveX (const float &value)
{
    pos += 0.05f * float (value) * (rot * glm::vec3 (1, 0, 0));
}

void Camera::MoveY (const float &value)
{
    pos += 0.05f * float (value) * (rot * glm::vec3 (0, 1, 0));
}
