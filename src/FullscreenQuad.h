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
#ifndef FULLSCREENQUAD_H
#define FULLSCREENQUAD_H

#include "common.h"
#include "Texture.h"

/** Fullscreen quad.
 * This class renders a full screen quad.
 */
class FullscreenQuad
{
public:
	/** Obtain class instance.
	 * FullscreenQuad is a singleton class, so it can't be constructed,
	 * but this function is called to obtain a reference to a global object.
	 * \returns a reference to the global fullscreen quad object
	 *
	 */
	static const FullscreenQuad &Get (void);
	/** Release global object.
	 * FullscreenQuad is a singleton class, so there is a global object that
	 * is created on demand. On application termination this object has to be
	 * released by calling this function.
	 */
	static void Release (void);
    /** Render.
     * Renders the framing.
     */
    void Render (void) const;
private:
    /** Global object.
     * The FullscreenQuad is a singleton class, so there exists only one global instance
     * of this class at any time, which is stored in this static variable.
     *
     */
    static FullscreenQuad *object;
    /** Private constructor.
     * As FullscreenQuad is a singleton class the constructor is private and only used internally
     * to create the global object.
     */
	FullscreenQuad (void);
    /** Destructor.
     * As FullscreenQuad is a singleton class the destructor is private and only used internally
     * to release the global object.
     */
    ~FullscreenQuad (void);
    union {
        struct {
            /** Vertex buffer.
             *  OpenGL buffer object storing the vertices of the framing.
             */
            GLuint vertexbuffer;
            /** Index buffer.
             * OpenGL buffer object storing the vertex indices.
             */
            GLuint indexbuffer;
        };
        /** Buffer objects.
         * The buffer objects are stored in a union, so that it is possible
         * to create/delete all buffer objects with a single OpenGL call.
         */
        GLuint buffers[2];
    };
    /** Vertex array object.
     * OpenGL vertex array object used to store information about the layout and location
     * of the vertex attributes.
     */
    GLuint vertexarray;
};

#endif /* FULLSCREENQUAD_H */
