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
#include "Skybox.h"

Skybox::Skybox (void)
{
	// load shader
    program.CompileShader (GL_VERTEX_SHADER, "shaders/skybox/vertex.glsl");
    program.CompileShader (GL_FRAGMENT_SHADER, "shaders/skybox/fragment.glsl");
    program.Link ();

    // create and bind a vertex array
    glGenVertexArrays (1, &vertexarray);
    glBindVertexArray (vertexarray);

    // generate buffer objects
    glGenBuffers (4, buffers);

    // store vertices to a buffer object
    glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
    const GLfloat vertices[] = {
            -1, -1, 1,
            1, -1, 1,
            1, -1, -1,
            -1, -1, -1,

            -1, 1, 1,
            1, 1, 1,
            1, 1, -1,
            -1, 1, -1,
    };
    glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
    // define the vertices as vertex attribute 0
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray (0);

    // store indices to a buffer object
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    const GLushort indices[] = {
            0, 1, 2,
            0, 2, 3,

            6, 5, 4,
            7, 6, 4,

            1, 0, 4,
            1, 4, 5,

            2, 1, 6,
            1, 5, 6,

            3, 2, 6,
            3, 6, 7,

            0, 3, 7,
            0, 7, 4
    };
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);
}

Skybox::~Skybox (void)
{
    // clean up
    glDeleteBuffers (4, buffers);
    glDeleteVertexArrays (1, &vertexarray);
}

void Skybox::Render (const Texture &envmap)
{
	// activate shader program
	program.Use ();
    // bind texture, vertex array and index buffer
    envmap.Bind (GL_TEXTURE_CUBE_MAP);
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    // render the framing
    glDrawElements (GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
}
