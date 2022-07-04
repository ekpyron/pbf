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
#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"

/** Camera class.
 * The camera class processes user input in order to create a view matrix for the scene.
 */
class Camera
{
public:
    /** Generate a view matrix.
     * Generates and returns a view matrix for the current camera position and orientation.
     * \returns the generated view matrix
     */
    glm::mat4 GetViewMatrix (void) const;

    /** Get the camera position.
     * \returns the current position of the camera
     */
    const glm::vec3 &GetPosition (void) const;

    /** Rotate the camera.
     * Rotates the camera around the specified angles.
     * \param xangle rotation around the x axis
     * \param yangle rotation around the y axis
     */
    void Rotate (float xangle, float yangle);
    /** %Camera zoom.
     * Zooms the camera by the specified amount.
     * \param value the zoom factor
     */
    void Zoom (float value);
    /** Move the camera in x direction.
     * Moves the camera in the direction of the x axis of its current coordinate system.
     * \param value distance to move
     */
    void MoveX (float value);
    /** Move the camera in y direction.
     * Moves the camera in the direction of the y axis of its current coordinate system.
     * \param value distance to move
     */
    void MoveY (float value);

    /** Set the camera position.
     * Specify the camera position.
     * \param pos new camera position
     */
    void SetPosition (const glm::vec3 &pos);
private:
    /** Current position of the camera.
     */
    glm::vec3 pos;
    /** Current rotation of the camera.
     *  Stores the current rotation of the camera as a quaternion.
     */
    glm::quat rot{1, 0, 0, 0};
    /** Rotation around the x axis.
     * Stores the current rotation of the camera around the x axis.
     */
    float xangle = 0.0f;
    /** Rotation around the y axis.
     * Stores the current rotation of the camera around the y axis.
     */
    float yangle = 0.0f;
};

#endif /* CAMERA_H */
