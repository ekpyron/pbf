#ifndef SIMULATION_H
#define SIMULATION_H

#include "common.h"
#include "Camera.h"
#include "ShaderProgram.h"
#include "Framing.h"
#include "Font.h"

/** Simulation class.
 * This is the main class which takes care of the whole simulation.
 */
class Simulation
{
public:
    /** Constructor.
     */
    Simulation (void);
    /** Destructor.
     */
    ~Simulation (void);

    /** Mouse movement event.
     * Handles mouse movement events.
     * \param x relative movement of the cursor in x direction
     * \param y relative movement of the cursor in y direction
     */
    void OnMouseMove (const double &x, const double &y);

    /** Resize event.
     * Handles window resize event.
     * \param width new framebuffer width
     * \param height new framebuffer height
     */
    void Resize (const unsigned int &width, const unsigned int &height);

    /** Animation loop.
     * This function is called every frame and performs the actual simulation.
     */
    void Frame (void);
private:
    /** Camera.
     * Used to handle input events and create a view matrix.
     */
    Camera camera;
    /** Shader program.
     * Shader program for displaying the scene.
    */
    ShaderProgram renderprogram;

    /** Framing.
     * Takes care of rendering a framing for the scene.
     */
    Framing framing;

    /** View matrix uniform location.
     * Uniform location of the view matrix in the render program.
     */
    GLint viewmat_location;
    /** Projection matrix uniform location.
     * Uniform location of the projection matrix in the render program.
     */
    GLint projmat_location;

    /** Font subsystem.
     * Takes care of displaying text.
     */
    Font font;

    /** Framebuffer width.
     * Width of the current framebuffer.
     */
    unsigned int width;
    /** Framebuffer height.
     * Height of the current framebuffer.
     */
    unsigned int height;

    /** Last FPS time.
     * Stores the last time the fps count was updated.
     */
    float last_fps_time;
    /** Frame counter.
     * Frame counter used to determine the frames per second.
     */
    unsigned int framecount;
    /** Frames per second.
     * The number of frames rendered in the last second.
     */
    unsigned int fps;
};

#endif /* SIMULATION_H */
