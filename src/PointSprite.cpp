#include "PointSprite.h"

PointSprite::PointSprite (void)
{
    // create and bind a vertex array
    glGenVertexArrays (1, &vertexarray);
    glBindVertexArray (vertexarray);

    // generate buffer objects
    glGenBuffers (2, buffers);

    GLshort vertices[] = {
    		-32767, 32767,
    		 32767, 32767,
    		 32767,-32767,
    		-32767,-32767
    };

    // store vertices to a buffer object
    glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
    // define the vertices as vertex attribute 0
    glVertexAttribPointer (0, 2, GL_SHORT, GL_TRUE, 0, 0);
    glEnableVertexAttribArray (0);

    const GLushort indices[] = {
    		0, 2, 1,
    		2, 0, 3
    };

    // store indices to a buffer object
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);
}

PointSprite::~PointSprite (void)
{
    // cleanup
    glDeleteBuffers (2, buffers);
    glDeleteVertexArrays (1, &vertexarray);
}

void PointSprite::SetPositionBuffer (GLuint buffer, GLsizei stride, GLintptr offset)
{
    // bind the vertex array and the position buffer
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ARRAY_BUFFER, buffer);
    // define the per-instance vertex attribute
    glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, stride, (const GLvoid*) offset);
    glEnableVertexAttribArray (1);
    glVertexAttribDivisor (1, 1);
}

void PointSprite::SetHighlightBuffer (GLuint buffer, GLsizei stride, GLintptr offset)
{
    // bind the vertex array and the position buffer
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ARRAY_BUFFER, buffer);
    // define the per-instance vertex attribute
    glVertexAttribIPointer (2, 1, GL_UNSIGNED_INT, stride, (const GLvoid*) offset);
    glEnableVertexAttribArray (2);
    glVertexAttribDivisor (2, 1);
}

void PointSprite::Render (GLuint instances) const
{
    // bind vertex array and index buffer
    glBindVertexArray (vertexarray);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    // render the framing
    glDrawElementsInstanced (GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0, instances);
}
