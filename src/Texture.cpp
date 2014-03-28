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
#include "Texture.h"
#include <png.h>

Texture::Texture (void)
{
    glGenTextures (1, &texture);
}

Texture::~Texture (void)
{
    glDeleteTextures (1, &texture);
}

void Texture::Bind (GLenum target) const
{
    glBindTexture (target, texture);
}

/** libpng I/O helper callback.
 * File I/O callback helper for using libpng with STL file streams.
 * \param png_ptr libpng handle
 * \param data memory to which to put the data
 * \param length number of bytes to read
 *
 */
void _PngReadFn (png_structp png_ptr, png_bytep data, png_size_t length)
{
    std::ifstream *file = reinterpret_cast<std::ifstream*> (png_get_io_ptr (png_ptr));
    file->read (reinterpret_cast<char*> (data), length);
    if (file->fail ())
        png_error (png_ptr, "I/O error.");
}

void Texture::Load (const GLenum &target, const std::string &filename, GLuint internalformat)
{
    // open the file
    std::ifstream file (filename.c_str (), std::ios_base::in|std::ios_base::binary);
    if (!file.is_open ())
        throw std::runtime_error (std::string ("Cannot open texture: ") + filename);

    png_structp png_ptr;
    png_infop info_ptr;

    // initialize libpng
    png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING,
        NULL, NULL, NULL);
    if (!png_ptr)
        throw std::runtime_error ("Cannot create png read struct");
    info_ptr = png_create_info_struct (png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct (&png_ptr, NULL, NULL);
        throw std::runtime_error ("Cannot create png info struct.");
    }

    // long jump error handling
    if (setjmp (png_jmpbuf (png_ptr)))
    {
        png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
        throw std::runtime_error ("libpng error.");
    }

    // set I/O callback
    png_set_read_fn (png_ptr, &file, _PngReadFn);

    // read header information and request a usable format
    png_read_info (png_ptr, info_ptr);
    png_set_packing (png_ptr);
    png_set_expand (png_ptr);
    if (png_get_bit_depth (png_ptr, info_ptr) == 16)
        png_set_swap (png_ptr);
    png_read_update_info (png_ptr, info_ptr);

    // obtain information about the image
    int rowbytes = png_get_rowbytes (png_ptr, info_ptr);
    int channels= png_get_channels (png_ptr, info_ptr);
    int width = png_get_image_width (png_ptr, info_ptr);
    int height = png_get_image_height (png_ptr, info_ptr);
    int depth = png_get_bit_depth (png_ptr, info_ptr);

    // assure a valid pixel depth
    if (depth != 8 && depth != 16 && depth != 32)
    {
        png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
        throw std::runtime_error (std::string ("Invalid bit depth in ") + filename);
    }

    // convert depth to OpenGL data type
    const GLuint types[] = {
        GL_UNSIGNED_BYTE,
        GL_UNSIGNED_SHORT,
        0,
        GL_UNSIGNED_INT
    };
    GLuint type = types [(depth / 8) - 1];

    // assure a valid number of channels
    if (channels < 1 || channels > 4)
    {
        png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
        throw std::runtime_error (std::string ("Invalid number of channels.") + filename);
    }

    // convert the number of channels to an OpenGL format
    const GLuint formats[] = {
        GL_RED, GL_RG, GL_RGB, GL_RGBA
    };
    GLuint format = formats[channels - 1];

    // read the image data
    std::vector<GLubyte> data;
    data.resize (rowbytes * height);
    int i;
    for (i = 0; i < height; i++)
    {
        png_read_row (png_ptr, &data[i * rowbytes], NULL);
    }

    // cleanup libpng
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

    // pass the image data to OpenGL
    glTexImage2D (target, 0, internalformat, width, height, 0, format, type, &data[0]);
}
