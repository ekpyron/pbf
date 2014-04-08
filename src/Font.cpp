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
#include "Font.h"

Font::Font (const std::string &filename)
{
   // create and bind a vertex array
    glGenVertexArrays (1, &vertexarray);
    glBindVertexArray (vertexarray);

    // generate buffer objects
    glGenBuffers (2, buffers);

    // store vertices to the vertex buffer
    glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
    const GLushort vertices[] = {
            0, 0,
            1, 0,
            1, 1,
            0, 1
    };
    glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
    // define the vertices as vertex attribute 0
    glVertexAttribPointer (0, 2, GL_UNSIGNED_SHORT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray (0);

    // store the indices to the index buffer
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    const GLushort indices[] = {
            1, 0, 2,
            2, 0, 3
    };
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);

    // load the texture (only the red channel needs to be stored internally)
    texture.Bind (GL_TEXTURE_2D);
    Texture::Load (GL_TEXTURE_2D, filename, GL_COMPRESSED_RED);
    // generate mipmap and activate trilinear filtering
    glGenerateMipmap (GL_TEXTURE_2D);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // compile the shader program
    program.CompileShader (GL_VERTEX_SHADER, "shaders/font/vertex.glsl");
    program.CompileShader (GL_FRAGMENT_SHADER, "shaders/font/fragment.glsl");
    program.Link ();

    // get the uniform locations
    mat_location = program.GetUniformLocation ("mat");
    charpos_location = program.GetUniformLocation ("charpos");
    pos_location = program.GetUniformLocation ("pos");

    // specify an orthographic transformation matrix
    glm::mat4 mat = glm::ortho (0.0f, 40.0f, 25.0f, 0.0f);
    glProgramUniformMatrix4fv (program.get (), mat_location, 1, GL_FALSE, glm::value_ptr (mat));
}

Font::~Font (void)
{
    // clean up
    glDeleteBuffers (2, buffers);
    glDeleteVertexArrays (1, &vertexarray);
}

void Font::PrintStr (const float &x, const float &y, const std::string &text)
{
    // bind the shader program, texture, vertex array and index buffer
    program.Use ();
    texture.Bind (GL_TEXTURE_2D);
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);

    // enable additive blending
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE);
    glBlendEquation (GL_FUNC_ADD);
    // disable depth testing
    glDisable (GL_DEPTH_TEST);

    // display the string
    for (size_t i = 0; i < text.length (); i++)
    {
        // calculate the position of the character in the font texture and pass it to the shader
        char c = text[i];
        if (c < 0x20 || c > 0x7f)
            continue;
        c -= 0x20;
        glUniform2f (charpos_location, float (c & 0xF) / 16.0f, float (c>>4) / 8.0f);
        // pass the position at which to display the character to the shader
        glUniform2f (pos_location, x + float(i)/2, y);
        // draw the "square"
        glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    }
    // disable blending
    glDisable (GL_BLEND);
    // enable depth test
    glEnable (GL_DEPTH_TEST);
}
