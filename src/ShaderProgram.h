#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include "common.h"

/** Shader program.
 * A class to create and store an OpenGL shader program.
 */
class ShaderProgram
{
public:
    /** Constructor.
     */
    ShaderProgram (void);
    /** Destructor.
     */
    ~ShaderProgram (void);

    /** Compile a shader.
     * Compiles a shader of given type and attaches it to the shader program.
     * \param type type of the shader to attach
     * \param filename path to the shader source
     */
    void CompileShader (GLenum type, const std::string &filename);
    /** Link the shader program.
     * Links the shader program.
     */
    void Link (void);
    /** Use the shader program.
     * Installs the program as part of the current rendering state.
     */
    void Use (void) const;

    /** Get a uniform location.
     * Obtains the location of a uniform of the shader program.
     * \param name name of the uniform variable
     * \returns the uniform location
     */
    GLint GetUniformLocation (const char *name) const;

    /** Get the program object.
     * \returns the OpenGL shader program object.
     */
    const GLuint &get (void) const {
        return program;
    }
private:
    /** Program object.
     * OpenGL shader program object.
     */
    GLuint program;
};

#endif /* SHADERPROGRAM_H */
