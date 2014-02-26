#ifndef SELECTION_H
#define SELECTION_H

#include "common.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "PointSprite.h"

class Selection
{
public:
	/** Constructor.
	 */
	Selection (void);
	/** Destruction.
	 */
	~Selection (void);

	/** Get particle from screen position.
	 * Determines the particle at the given screen coordinates, if any.
	 * \param positionbuffer buffer object storing the particle positions
	 * \param numparticles number of particles in the buffer
	 * \param x x coordinate
	 * \param y y coordinate
	 * \returns the particle id or -1 if there is no particle at the given position
	 */
	GLint GetParticle (GLuint positionbuffer, GLuint numparticles, float x, float y);
private:
    /** Selection shader program.
     * Shader program for selecting particles.
     */
    ShaderProgram selectionprogram;

    /** Depth texture.
     * Texture for the depth buffer of the selection framebuffer.
     */
    Texture depthtexture;

    /** Selection texture.
     * Texture for the selection framebuffer.
     */
    Texture selectiontexture;

    union {
    	struct {
    		/** Selection framebuffer.
    		 * Framebuffer object used to determine which object resides at a specific screen
    		 * space position.
    		 */
    		GLuint selectionfb;
    	};
    	/** Framebuffer objects.
    	 * The framebuffer objects are stored in a union, so that it is possible
    	 * to create/delete all texture objects with a single OpenGL call.
    	 */
    	GLuint framebuffers[1];
    };

    PointSprite pointsprite;
};

#endif /* SELECTION_H */
