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
#ifndef PONTSPRITE_H
#define PONTSPRITE_H

#include "common.h"

/** PointSprite class.
 * This class renders point sprites that can be used for rendering particles.
 */
class PointSprite
{
public:
    /** Constructor.
     */
	PointSprite (void);
    /** Destructor
     */
    ~PointSprite (void);

    /** Specify a position buffer.
     * Specifies a buffer containing the positions for the point sprites.
     * \param buffer the buffer object containing the position data
     * \param stride number of bytes between consecutive positions or 0 for tightly packed positions
     * \param offset byte offset of the position data in the buffer object
     */
    void SetPositionBuffer (GLuint buffer, GLsizei stride = 0, GLintptr offset = 0);

    /** Specify a highlight buffer.
     * Specifies a buffer containing the highlighting information for the point sprites.
     * \param buffer the buffer object containing the highlighting information.
     * \param stride number of bytes between consecutive positions or 0 for tightly packed positions
     * \param offset byte offset of the position data in the buffer object
     */
    void SetHighlightBuffer (GLuint buffer, GLsizei stride = 0, GLintptr offset = 0);

    /** Render the point sprites.
     * \param instances number of instances to render.
     */
    void Render (GLuint instances) const;
private:
    union {
        struct {
            /** Vertex buffer.
             *  OpenGL buffer object storing the vertices of the point sprites.
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

#endif /* SPHERE_H */
