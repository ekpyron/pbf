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
#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include "common.h"

/** Shader program.
 * A class to create and store an OpenGL shader program.
 */
class ShaderProgram {
public:
    /** Constructor.
     */
    ShaderProgram(void);

    /** Destructor.
     */
    ~ShaderProgram(void);

    /** Compile a shader.
     * Compiles a shader of given type and attaches it to the shader program.
     * \param type type of the shader to attach
     * \param filename path to the shader source
     * \param header optional header to include at the top of the file
     */
    void CompileShader(GLenum type, const std::string &filename, const std::string &header = std::string()) {
        CompileShader(type, {filename}, header);
    }

    /** Compile a shader.
     * Compiles a shader of given type and attaches it to the shader program.
     * \param type type of the shader to attach
     * \param filename path to the shader source
     * \param header optional header to include at the top of the file
     */
    void
    CompileShader(GLenum type, std::initializer_list<std::string> filenames, const std::string &header = std::string());

    /** Link the shader program.
     * Links the shader program.
     */
    void Link(void);

    /** Use the shader program.
     * Installs the program as part of the current rendering state.
     */
    void Use(void) const;

    /** Get a uniform location.
     * Obtains the location of a uniform of the shader program.
     * \param name name of the uniform variable
     * \returns the uniform location
     */
    GLint GetUniformLocation(const char *name) const;

    /** Get the program object.
     * \returns the OpenGL shader program object.
     */
    const GLuint &get(void) const {
        return program;
    }

private:
    /** Program object.
     * OpenGL shader program object.
     */
    GLuint program;
};

#endif /* SHADERPROGRAM_H */
