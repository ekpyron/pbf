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
#ifndef SELECTION_H
#define SELECTION_H

#include "common.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "PointSprite.h"

/** Selection class.
 * This class is responsible for determining which particle is located at a given screen position.
 */
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

    /** Point sprite object.
     * This is used to render point sprites for the particles.
     */
    PointSprite pointsprite;
};

#endif /* SELECTION_H */
