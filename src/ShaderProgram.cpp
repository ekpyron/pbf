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
#include "ShaderProgram.h"

ShaderProgram::ShaderProgram(void) : program(glCreateProgram()) {
}

ShaderProgram::~ShaderProgram(void) {
    glDeleteProgram(program);
}

void ShaderProgram::Link(void) {
    // attempt to link the program
    glLinkProgram(program);

    // check for errors and throw the error log as exception on failure
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        std::vector<char> log;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        log.resize(length);
        glGetProgramInfoLog(program, length, NULL, &log[0]);
        throw std::runtime_error(std::string("Failed to link shader program: ") + std::string(&log[0], length));
    }
}

void ShaderProgram::Use(void) const {
    glUseProgram(program);
}

GLint ShaderProgram::GetUniformLocation(const char *name) const {
    return glGetUniformLocation(program, name);
}

void
ShaderProgram::CompileShader(GLenum type, std::initializer_list<std::string> filenames, const std::string &header) {
    GLuint shader;

    std::vector<char> data;
    GLint length = 0;

    // prepend #version statement
    const char version[] = "#version 430 core\n";
    data.assign(version, version + (sizeof(version) / sizeof(version[0]) - 1));

    // prepend header
    data.insert(data.end(), header.begin(), header.end());

    // fix line count
    const std::string linedef = "\n#line 1\n";
    data.insert(data.end(), linedef.begin(), linedef.end());

    // load shader source
    length = static_cast<GLint>(data.size());
    for (const auto &filename : filenames) {
        size_t len;
        std::ifstream f(filename.c_str(), std::ios_base::in);
        if (!f.is_open())
            throw std::runtime_error(std::string("Cannot load shader: ") + filename);

        f.seekg(0, std::ios_base::end);
        len = static_cast<std::size_t>(f.tellg());
        f.seekg(0, std::ios_base::beg);

        data.resize(length + len);
        f.read(&data[length], len);
        len = static_cast<std::size_t>(f.gcount());

        if (f.bad())
            throw std::runtime_error(std::string("Cannot load shader: ") + filename);
        length += len;
    }

    // create a shader, specify the source and attempt to compile it
    shader = glCreateShader(type);
    const GLchar *src = (const GLchar *) &data[0];
    glShaderSource(shader, 1, &src, &length);
    glCompileShader(shader);

    // check the compilation status and throw the error log as exception on failure
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        data.resize(length);
        glGetShaderInfoLog(shader, length, NULL, &data[0]);
        glDeleteShader(shader);
        std::string names;
        for (const auto &fname : filenames) {
            if (names.size() > 0) {
                names += ", ";
            }
            names += "\"" + fname + "\"";
        }
        throw std::runtime_error(fmt::format("Cannot compile shader {}: {}", names, std::string(&data[0], length)));
    }

    // attach the shader object to the program
    glAttachShader(program, shader);
    // delete the shader object (it is internally stored as long as the program is not deleted)
    glDeleteShader(shader);
}
