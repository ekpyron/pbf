#ifndef SKYBOX_H
#define SKYBOX_H

#include "common.h"
#include "Texture.h"
#include "ShaderProgram.h"

/** Sky box.
 * This class renders a sky box for the scene.
 */
class Skybox
{
public:
    /** Constructor.
     */
	Skybox (void);
    /** Destructor.
     */
    ~Skybox (void);
    /** Render.
     * Renders the skybox.
     * \param envmap Environment map to use on the sky box.
     */
    void Render (const Texture &envmap);
private:
    /** Shader program.
     * Shader program for displaying the sky box.
    */
    ShaderProgram program;

    union {
        struct {
            /** Vertex buffer.
             *  OpenGL buffer object storing the vertices of the sky box.
             */
            GLuint vertexbuffer;
            /** Normal buffer.
             *  OpenGL buffer object storing the normals of the sky box.
             */
            GLuint normalbuffer;
            /** Texture coordinate buffer.
             *  OpenGL buffer object storing the texture coordinates of the sky box.
             */
            GLuint texcoordbuffer;
            /** Index buffer.
             * OpenGL buffer object storing the vertex indices.
             */
            GLuint indexbuffer;
        };
        /** Buffer objects.
         * The buffer objects are stored in a union, so that it is possible
         * to create/delete all buffer objects with a single OpenGL call.
         */
        GLuint buffers[4];
    };
    /** Vertex array object.
     * OpenGL vertex array object used to store information about the layout and location
     * of the vertex attributes.
     */
    GLuint vertexarray;
};

#endif /* SKYBOX_H */
