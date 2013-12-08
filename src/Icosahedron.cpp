#include "Icosahedron.h"

Icosahedron::Icosahedron (void)
{
    // create and bind a vertex array
    glGenVertexArrays (1, &vertexarray);
    glBindVertexArray (vertexarray);

    // generate buffer objects
    glGenBuffers (2, buffers);

    // store vertices to a buffer object
    const GLshort vertices[]= {
            0, -17227, 27873,
            27873, 0, 17227,
            27873, 0, -17227,
            -27873, 0, -17227,
            -27873, 0, 17227,
            -17227, 27873, 0,
            17227, 27873, 0,
            17227, -27873, 0,
            -17227, -27873, 0,
            0, -17227, -27873,
            0, 17227, -27873,
            0, 17227, 27873
    };
    glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), &vertices[0], GL_STATIC_DRAW);
    // define the vertices as vertex attribute 0
    glVertexAttribPointer (0, 3, GL_SHORT, GL_TRUE, 0, 0);
    glEnableVertexAttribArray (0);

    // store indices to a buffer object
    static const GLubyte indices[] = {
            1, 2, 6,
            1, 7, 2,
            3, 4, 5,
            4, 3, 8,
            6, 5, 11,
            5, 6, 10,
            9, 10, 2,
            10, 9, 3,
            7, 8, 9,
            8, 7, 0,
            11, 0, 1,
            0, 11, 4,
            6, 2, 10,
            1, 6, 11,
            3, 5, 10,
            5, 4, 11,
            2, 7, 9,
            7, 1, 0,
            3, 9, 8,
            4, 8, 0,
    };
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), &indices[0], GL_STATIC_DRAW);
}

void Icosahedron::SetPositionBuffer (GLuint buffer)
{
    // bind the vertex array and the position buffer
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ARRAY_BUFFER, buffer);
    // define the per-instance vertex attribute
    glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray (1);
    glVertexAttribDivisor (1, 1);
}

Icosahedron::~Icosahedron (void)
{
    // cleanup
    glDeleteBuffers (2, buffers);
}


void Icosahedron::Render (GLuint instances) const
{
    // bind vertex array and index buffer
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    // render the framing
    glDrawElementsInstanced (GL_TRIANGLES, 20 * 3, GL_UNSIGNED_BYTE, 0, instances);
}
