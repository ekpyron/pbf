#ifndef FONT_H
#define FONT_H

#include "common.h"
#include "Texture.h"
#include "ShaderProgram.h"

/** Font subsystem.
 * A simple class for displaying text using a font texture.
 */
class Font
{
public:
    /** Constructor.
     * \param filename the filename of the font texture to load
     */
    Font (const std::string &filename);
    /** Destructor.
     */
    ~Font (void);

    /** Print string.
     * Displays a string at the given position.
     * \param x x coordinate at which to display the text
     * \param y y coordinate at which to display the text
     * \param text text to display
     */
    void PrintStr (const float &x, const float &y, const std::string &text);
private:
    union {
        struct {
            /** Vertex buffer.
             *  OpenGL buffer object storing the vertices of a square.
             */
            GLuint vertexbuffer;
            /** Index buffer.
             * OpenGL buffer object storing the vertex indices.
             */
            GLuint indexbuffer;
        };
        /** Buffer objects.
         * The buffer objects are stored in a union, so that it is possible
         * to create/delete all buffer objects with a single OpenGL call.
         */
        GLuint buffers[2];
    };
    /** Vertex array object.
     * OpenGL vertex array object used to store information about the layout and location
     * of the vertex attributes.
     */
    GLuint vertexarray;
    /** Font texture.
     * Texture object to store the font texture.
     */
    Texture texture;
    /** Matrix uniform location.
     * Uniform location of the transformation matrix.
     */
    GLuint mat_location;
    /** Character position uniform location.
     * Uniform location of the character position, i.e. the position
     * of the character to be drawn within the font texture.
     */
    GLuint charpos_location;
    /** Position uniform location.
     * Uniform location of the raster position at which a character should
     * be drawn.
     */
    GLuint pos_location;
    /** Shader program.
     * Shader program for the shaders used to display the text.
     */
    ShaderProgram program;
};

#endif /* FONT_H */
