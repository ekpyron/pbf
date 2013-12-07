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
    const GLubyte vertices[] = {
            0, 0,
            1, 0,
            1, 1,
            0, 1
    };
    glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
    // define the vertices as vertex attribute 0
    glVertexAttribPointer (0, 2, GL_UNSIGNED_BYTE, GL_FALSE, 0, 0);
    glEnableVertexAttribArray (0);

    // store the indices to the index buffer
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    const GLubyte indices[] = {
            0, 1, 2,
            0, 2, 3
    };
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);

    // load the texture (only the red channel needs to be stored internally)
    texture.Bind (GL_TEXTURE_2D);
    texture.Load (filename, GL_COMPRESSED_RED);
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
        glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
    }
    // disable blending
    glDisable (GL_BLEND);
}
