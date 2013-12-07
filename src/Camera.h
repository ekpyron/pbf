#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"

/** Camera class.
 * The camera class processes user input in order to create a view matrix for the scene.
 */
class Camera
{
public:
    /** Constructor.
     */
    Camera (void);
    /** Destructor.
     */
    ~Camera (void);

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
    void Rotate (const float &xangle, const float &yangle);
    /** %Camera zoom.
     * Zooms the camera by the specified amount.
     * \param value the zoom factor
     */
    void Zoom (const float &value);
    /** Move the camera in x direction.
     * Moves the camera in the direction of the x axis of its current coordinate system.
     * \param value distance to move
     */
    void MoveX (const float &value);
    /** Move the camera in y direction.
     * Moves the camera in the direction of the y axis of its current coordinate system.
     * \param value distance to move
     */
    void MoveY (const float &value);

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
    glm::quat rot;
    /** Rotation around the x axis.
     * Stores the current rotation of the camera around the x axis.
     */
    float xangle;
    /** Rotation around the y axis.
     * Stores the current rotation of the camera around the y axis.
     */
    float yangle;
};

#endif /* CAMERA_H */
