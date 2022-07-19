/*
 * Copyright (c) 2013-2014 Daniel Kirchner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE ANDNONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "Camera.h"

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
