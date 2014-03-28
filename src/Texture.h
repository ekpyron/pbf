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
#ifndef TEXTURE_H
#define TEXTURE_H

#include "common.h"

/** Texture class.
 * This class stores an OpenGL texture object and is capably of loading png images.
 */
class Texture
{
public:
    /** Constructor.
     */
    Texture (void);
    /** Destructor.
     */
    ~Texture (void);

    /** Bind the texture.
     * Bind the texture to the specified texturing target.
     * \param target texturing target to which to bind the texture
     */
    void Bind (GLenum target) const;

    /** Load a png image.
     * Loads a png image and stores it in the texture bound to the specified target.
     * \param target specifies the texture target
     * \param filename path to the image to load
     * \param internalformat internal format used to store the texture
     */
    static void Load (const GLenum &target, const std::string &filename, GLuint internalformat = GL_RGBA8);

    /** Get the texture object.
     * \returns the OpenGL texture object
     */
    const GLuint &get (void) const {
        return texture;
    }

private:
    /** Texture object.
     * The OpenGL texture object managed by this class.
     */
    GLuint texture;
};

#endif /* TEXTURE_H */
