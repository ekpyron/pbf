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
