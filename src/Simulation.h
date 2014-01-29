#ifndef SIMULATION_H
#define SIMULATION_H

#include "common.h"
#include "Camera.h"
#include "ShaderProgram.h"
#include "Framing.h"
#include "Font.h"
#include "RadixSort.h"
#include "FullscreenQuad.h"
#include "PointSprite.h"

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
     */
    bool Frame (void);
private:
    typedef struct transformationbuffer {
    	glm::mat4 viewmat;
    	glm::mat4 projmat;
    	glm::mat4 invviewmat;
    } transformationbuffer_t;
    /** Update view matrix.
     * Called whenever the view matrix is changed.
     */
    void UpdateViewMatrix (void);
    /** Camera.
     * Used to handle input events and create a view matrix.
     */
    Camera camera;

    /** Surrounding shader program.
     * Shader program for displaying the surrounding scene.
    */
    ShaderProgram surroundingprogram;

    /** Particle shader program.
     * Shader program for displaying the particles.
    */
    ShaderProgram particleprogram;

    /** Particle depth shader program.
     * Shader program for rendering the particle depths only.
    */
    ShaderProgram particledepthprogram;

    /** Selection shader program.
     * Shader program for selecting particles.
     */
    ShaderProgram selectionprogram;

    /** Simulation step shader program.
     * Shader program for the simulation step that predicts the new position
     * of all particles.
     */
    ShaderProgram predictpos;

    /** Simulation step shader program.
     * Shader program for the simulation step that finds grid cells in the
     * sorted particle array.
     */
    ShaderProgram findcells;

    /** Simulation step shader program.
     * Shader program for the simulation step that calculates lambda_i for each particle.
     */
    ShaderProgram calclambda;

    /** Simulation step shader program.
     * Shader program for the simulation step that updates the position for each particle.
     */
    ShaderProgram updatepos;

    /** Fullscreen quad shader program.
     * Shader program for rendering the fullscreen quad.
     */
    ShaderProgram fsquadprog;

    /** Depth blur shader program.
     * Shader program for blurring the particle depth texture.
     */
    ShaderProgram depthblurprog;

    /** Depth blur direction uniform location.
     * Uniform location for the uniform variable storing the direction of the depth blur.
     */
    GLuint depthblurdir;

    /** Framing.
     * Takes care of rendering a framing for the scene.
     */
    Framing framing;

    /** Point Sprite.
     * Takes care of rendering particles as point sprites.
     */
    PointSprite pointsprite;

    /** Sphere flag.
     * Flag indicating whether to use spheres or icosahedra for
     * particle rendering.
     */
    bool usespheres;

    /** Surface reconstruction flag.
     * flag indicating whether the particles should be rendered
     * as spheres or whether a reconstructed water surface should
     * be rendered.
     */
    bool surfacereconstruction;

    /** Radix sort.
     * Takes care of sorting the particle list.
     * The contained buffer object is used as particle buffer.
     */
    RadixSort radixsort;

    union {
        struct {
            /** Lambda buffer.
             * Buffer in which the specific scalar values are stored during the simulation steps.
             */
            GLuint lambdabuffer;
            /** Transformation uniform buffer.
             * Buffer object to store the projection and view matrix.
             */
            GLuint transformationbuffer;

            /** Lighting uniform buffer.
             * Buffer object to store the lighting parameters.
             */
            GLuint lightingbuffer;

            /** Auxiliary buffer.
             * Buffer in which auxiliary data used for debugging purposes can be stored.
             */
            GLuint auxbuffer;

            /** Grid buffer.
             * Buffer in which the starting particle for each grid is stored.
             */
            GLuint gridbuffer;

            /** Flag buffer.
             * Buffer in which a flag is stored that indicates the grid cell borders in the particle buffer.
             */
            GLuint flagbuffer;
        };
        /** Buffer objects.
         * The buffer objects are stored in a union, so that it is possible
         * to create/delete all buffer objects with a single OpenGL call.
         */
        GLuint buffers[6];
    };

    union {
    	struct {
    		/** Selection framebuffer.
    		 * Framebuffer object used to determine which object resides at a specific screen
    		 * space position.
    		 */
    		GLuint selectionfb;
    		/** Depth framebuffer.
    		 * Framebuffer object used to store the particle depths.
    		 */
    		GLuint depthfb;
    		/** Horizontal depth blur framebuffer.
    		 * Framebuffer object used for blurring the particle depths horizontally.
    		 */
    		GLuint depthhblurfb;
    		/** Vertical depth blur framebuffer.
    		 * Framebuffer object used for blurring the particle depths vertically.
    		 */
    		GLuint depthvblurfb;
    	};
    	/** Framebuffer objects.
    	 * The framebuffer objects are stored in a union, so that it is possible
    	 * to create/delete all texture objects with a single OpenGL call.
    	 */
    	GLuint framebuffers[4];
    };

    /** Font subsystem.
     * Takes care of displaying text.
     */
    Font font;

    union {
    	struct {
    		/** Flag texture.
    		 * Texture through which the flag buffer is accessed.
    		 */
    		GLuint flagtexture;

    		/** Selection depth texture.
    		 * Texture for the depth buffer of the selection framebuffer.
    		 */
    		GLuint selectiondepthtexture;

    		/** Particle depth texture.
    		 * Texture to store the particle depths.
    		 */
    		GLuint depthtexture;

    		/** Temporary blur texture.
    		 * Texture to store temporary steps during blurring.
    		 */
    		GLuint blurtexture;
    	};
    	/** Texture objects.
    	 * The texture objects are stored in a union, so that it is possible
    	 * to create/delete all texture objects with a single OpenGL call.
    	 */
    	GLuint textures[4];
    };

    /** Queries.
     * Queries used to determine the time spent in the different stages
     * of the algorithm.
     */
    GLuint queries[6];

    /** Fullscreen quad.
     * Renders a fullscreen quad.
     */
    FullscreenQuad fullscreenquad;

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

    /** Offscreen framebuffer width.
     * Width of the offscreen framebuffer.
     */
    unsigned int offscreen_width;
    /** Offscreen framebuffer height.
     * Height of the offscreen framebuffer.
     */
    unsigned int offscreen_height;

    /** Last frame time.
     * Stores the time when the rendering of the last frame took place.
     */
    float last_time;

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
