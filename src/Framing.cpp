#include "Framing.h"

Framing::Framing (void)
{
    // create and bind a vertex array
    glGenVertexArrays (1, &vertexarray);
    glBindVertexArray (vertexarray);

    // generate buffer objects
    glGenBuffers (4, buffers);

    // store vertices to a buffer object
    glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
    const GLfloat vertices[] = {
            -100, 0, 100,
            100, 0, 100,
            100, 0, -100,
            -100, 0, -100
    };
    glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
    // define the vertices as vertex attribute 0
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray (0);

    // store texture coordinates to a buffer object
    glBindBuffer (GL_ARRAY_BUFFER, texcoordbuffer);
    const GLushort texcoords[] = {
            0, 10,
            10, 10,
            10, 0,
            0, 0
    };
    glBufferData (GL_ARRAY_BUFFER, sizeof (texcoords), texcoords, GL_STATIC_DRAW);
    // define texture coordinates as vertex attribute 1
    glVertexAttribPointer (1, 2, GL_UNSIGNED_SHORT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray (1);

    // store normals to a buffer object
    glBindBuffer (GL_ARRAY_BUFFER, normalbuffer);
    const GLbyte normals[] = {
            0, 127, 0, 0,
            0, 127, 0, 0,
            0, 127, 0, 0,
            0, 127, 0, 0
    };
    glBufferData (GL_ARRAY_BUFFER, sizeof (normals), normals, GL_STATIC_DRAW);
    // define normals as vertex attribute 2
    glVertexAttribPointer (2, 3, GL_BYTE, GL_TRUE, 4, 0);
    glEnableVertexAttribArray (2);

    // store indices to a buffer object
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    const GLushort indices[] = {
            0, 1, 2,
            0, 2, 3
    };
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);

    // load the texture
    texture.Load ("textures/framing.png", GL_COMPRESSED_RGB);
    // generate a mipmap and enable trilinear filtering
    glGenerateMipmap (GL_TEXTURE_2D);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Framing::~Framing (void)
{
    // clean up
    glDeleteBuffers (4, buffers);
    glDeleteVertexArrays (1, &vertexarray);
}

void Framing::Render (void)
{
    // bind texture, vertex array and index buffer
    texture.Bind (GL_TEXTURE_2D);
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    // render the framing
    glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}
