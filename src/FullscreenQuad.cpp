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
#include "FullscreenQuad.h"

FullscreenQuad *FullscreenQuad::object = NULL;

const FullscreenQuad &FullscreenQuad::Get (void)
{
	if (object == NULL)
		object = new FullscreenQuad ();
	return *object;
}

void FullscreenQuad::Release (void)
{
	if (object != NULL)
	{
		delete object;
		object = NULL;
	}
}

FullscreenQuad::FullscreenQuad (void)
{
    // create and bind a vertex array
    glGenVertexArrays (1, &vertexarray);
    glBindVertexArray (vertexarray);

    // generate buffer objects
    glGenBuffers (2, buffers);

    // store vertices to a buffer object
    glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
    const GLshort vertices[] = {
            -32767, 32767,
            32767, 32767,
            32767, -32767,
            -32767,-32767
    };
    glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
    // define the vertices as vertex attribute 0
    glVertexAttribPointer (0, 2, GL_SHORT, GL_TRUE, 0, 0);
    glEnableVertexAttribArray (0);

    // store indices to a buffer object
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    const GLushort indices[] = {
            2, 1, 0,
            3, 2, 0
    };
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);
}

FullscreenQuad::~FullscreenQuad (void)
{
    // clean up
    glDeleteBuffers (2, buffers);
    glDeleteVertexArrays (1, &vertexarray);
}

void FullscreenQuad::Render (void) const
{
    // bind vertex array and index buffer
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    // render the framing
    glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}
