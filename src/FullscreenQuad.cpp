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
