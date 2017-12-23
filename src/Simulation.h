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
#ifndef SIMULATION_H
#define SIMULATION_H

#include "common.h"
#include "Camera.h"
#include "ShaderProgram.h"
#include "Framing.h"
#include "Font.h"
#include "FullscreenQuad.h"
#include "PointSprite.h"
#include "SPH.h"
#include "SurfaceReconstruction.h"
#include "Skybox.h"
#include "Selection.h"

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

    /** Mouse button event.
     * Handles mouse button events.
     * \param button button that was pressed.
     */
    void OnMouseDown (const int &button);

    /** Mouse button event.
     * Handles mouse button events.
     * \param button button that was released.
     */
    void OnMouseUp (const int &button);

    /** Reset particle buffer.
     * Resets the particle buffer containing the particle information.
     */
    void ResetParticleBuffer (void);

    /** Key up event.
     * Handles key release events.
     * \param key key that was released
     */
    void OnKeyUp (int key);

    /** Key down event.
     * Handles key press events.
     * \param key key that was pressed
     */
    void OnKeyDown (int key);

    /** Resize event.
     * Handles window resize event.
     * \param width new framebuffer width
     * \param height new framebuffer height
     */
    void Resize (const unsigned int &width, const unsigned int &height);

    /** Get number of particles.
     * Obtains the number of particles in the simulation.
     * \returns the number of particles in the simulation.
     */
    const unsigned int GetNumberOfParticles (void) const;

    /** Animation loop.
     * This function is called every frame and performs the actual simulation.
     * \returns True, if there were no errors, false otherwise.
     */
    bool Frame (void);
private:
    /** Transformation buffer layout.
     * Structure representing the memory layout of the uniform buffer
     * containing the transformation information.
     */
    typedef struct transformationbuffer {
    	/** View matrix.
    	 * The view matrix.
    	 */
    	glm::mat4 viewmat;
    	/** Projection matrix.
    	 * The projection matrix.
    	 */
    	glm::mat4 projmat;
    	/** Inverse view matrix.
    	 * Inverse of the view matrix.
    	 */
    	glm::mat4 invviewmat;
    } transformationbuffer_t;

    /** Lighting parameter layout.
     * Structure representing the memory layout of the uniform buffer
     * containing the lighting parameters.
     */
    typedef struct lightparams {
    	/** Light position.
    	 * Position of the light source.
    	 */
        glm::vec3 lightpos;
        /** Padding.
         * Padding to create the correct memory layout.
         */
        float padding;
        /** Spot direction.
         * Direction of the spot light.
         */
        glm::vec3 spotdir;
        /** Padding.
         * Padding to create the correct memory layout.
         */
        float padding2;
        /** Eye position.
         * Eye/camera position used for specular lighting.
         */
        glm::vec3 eyepos;
        /** Spot exponent.
         * Spot exponent used to calculate the spot light falloff.
         */
        float spotexponent;
        /** Light intensity.
         * Intensity of the light source.
         */
        float lightintensity;
    } lightparams_t;

    /** Update view matrix.
     * Called whenever the view matrix is changed.
     */
    void UpdateViewMatrix (void);
    /** Camera.
     * Used to handle input events and create a view matrix.
     */
    Camera camera;

    /** Particle shader program.
     * Shader program for displaying the particles.
    */
    ShaderProgram particleprogram;

    /** Skybox.
     * Takes care of rendering a sky box for the scene.
     */
    Skybox skybox;

    /** Framing.
     * Takes care of rendering a framing for the scene.
     */
    Framing framing;

    /** Point Sprite.
     * Takes care of rendering particles as point sprites.
     */
    PointSprite pointsprite;

    /** SPH class.
     * Takes care of the SPH simulation.
     */
    SPH sph;

    /** Environment map.
     * Optional environment map containing a texture for the sky box.
     */
    Texture *envmap;

    /** Sky box flag.
     * Flag indicating whether to use a sky box.
     */
    bool useskybox;

    /** Noise flag.
     * Flag indicating whether to use noise.
     */
    bool usenoise;

    /** Surface reconstruction class.
     * Takes care of surface reconstruction and rendering.
     */
    SurfaceReconstruction surfacereconstruction;

    /** Running flag
     * Flag indicating whether the simulation is running.
     */
    bool running;

    /** Surface reconstruction flag.
     * flag indicating whether the particles should be rendered
     * as spheres or whether a reconstructed water surface should
     * be rendered.
     */
    bool usesurfacereconstruction;

    union {
        struct {
            /** Transformation uniform buffer.
             * Buffer object to store the projection and view matrix.
             */
            GLuint transformationbuffer;

            /** Lighting uniform buffer.
             * Buffer object to store the lighting parameters.
             */
            GLuint lightingbuffer;
        };
        /** Buffer objects.
         * The buffer objects are stored in a union, so that it is possible
         * to create/delete all buffer objects with a single OpenGL call.
         */
        GLuint buffers[2];
    };

    /** Selection subsystem.
     * Takes care of determining particle ids from screen positions. Used for particle highlighting.
     */
    Selection selection;

    /** Font subsystem.
     * Takes care of displaying text.
     */
    Font font;

	/** Rendering query object.
	 * Query object to record the time spent in the rendering phase.
	 */
    GLuint renderingquery;

    /** Projection matrix.
     * Matrix describing the perspective projection.
     */
    glm::mat4 projmat;

    /** Framebuffer width.
     * Width of the current display framebuffer.
     */
    unsigned int width;
    /** Framebuffer height.
     * Height of the current display framebuffer.
     */
    unsigned int height;

    /** Last frame time.
     * Stores the time when the rendering of the last frame took place.
     */
    double last_time;

    /** Last FPS time.
     * Stores the last time the fps count was updated.
     */
    double last_fps_time;

    /** GUI timer.
     * Timer storing the time left to display GUI information in seconds.
     */
    float guitimer;

    /** Enum type for gui states.
     */
    typedef enum guistate {
    	GUISTATE_REST_DENSITY = 0,
    	GUISTATE_CFM_EPSILON,
    	GUISTATE_GRAVITY,
    	GUISTATE_TIMESTEP,
    	GUISTATE_NUM_SOLVER_ITERATIONS,
    	GUISTATE_TENSILE_INSTABILITY_K,
    	GUISTATE_TENSILE_INSTABILITY_SCALE,
    	GUISTATE_XSPH_VISCOSITY,
    	GUISTATE_VORTICITY_EPSILON,
    	GUISTATE_NUM_STATES,
    } guistate_t;

    /** Gui states.
     * Stores the current state of the gui.
     */
    guistate_t guistate;
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
