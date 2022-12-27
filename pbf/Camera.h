#pragma once

#include "common.h"
#include "UIControlled.h"

namespace pbf {

/** Camera class.
 * The camera class processes user input in order to create a view matrix for the scene.
 */
class Camera: public UIControlled
{
public:
	Camera(Context& _context);
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

	glm::mat4 GetViewRot() const;
protected:
	void ui() override;
private:
	Context& context;
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

}
