#include "Sphere.h"

Sphere::Sphere (void)
{
    // create and bind a vertex array
    glGenVertexArrays (1, &vertexarray);
    glBindVertexArray (vertexarray);

    // generate buffer objects
    glGenBuffers (2, buffers);

    // generate vertices
    std::vector<GLshort> vertices;
    vertices.reserve (8 * 8 * 3);
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            glm::vec3  pos (cosf (2.0f * M_PI * float (j) / 7.0f) * sinf (M_PI * float (i) / 7.0f),
                    sinf (2.0f * M_PI * float (j) / 7.0f) * sinf (M_PI * float (i) / 7.0f),
                    sin (-M_PI_2 + M_PI * float (i) / 7.0f));
            vertices.push_back (pos.x * 32767);
            vertices.push_back (pos.y * 32767);
            vertices.push_back (pos.z * 32767);
        }
    }

    // generate indices
    std::vector<GLubyte> indices;
    indices.reserve (8 * 8 * 6);
    for (int i = 0; i < 7; i++)
    {
        for (int j = 0; j < 7; j++)
        {
            indices.push_back (i * 8 + j);
            indices.push_back (i * 8 + j + 1);
            indices.push_back ((i + 1) * 8 + j + 1);

            indices.push_back (i * 8 + j);
            indices.push_back ((i + 1) * 8 + j + 1);
            indices.push_back ((i + 1) * 8 + j);
        }
    }

    // store vertices to a buffer object
    glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData (GL_ARRAY_BUFFER, sizeof (GLushort) * vertices.size (), &vertices[0], GL_STATIC_DRAW);
    // define the vertices as vertex attribute 0
    glVertexAttribPointer (0, 3, GL_SHORT, GL_TRUE, 0, 0);
    glEnableVertexAttribArray (0);

    // store indices to a buffer object
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (GLubyte) * indices.size (), &indices[0], GL_STATIC_DRAW);
}

void Sphere::SetPositionBuffer (GLuint buffer)
{
    // bind the vertex array and the position buffer
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ARRAY_BUFFER, buffer);
    // define the per-instance vertex attribute
    glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray (1);
    glVertexAttribDivisor (1, 1);
}

Sphere::~Sphere (void)
{
    // cleanup
    glDeleteBuffers (2, buffers);
}


void Sphere::Render (GLuint instances) const
{
    // bind vertex array and index buffer
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    // render the framing
    glDrawElementsInstanced (GL_TRIANGLES, 8 * 8 * 6, GL_UNSIGNED_BYTE, 0, instances);
}
