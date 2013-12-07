#ifndef FRAMING_H
#define FRAMING_H

#include "common.h"
#include "Texture.h"

/** Scene framing.
 * This class is renders a framing for the scene.
 */
class Framing
{
public:
    /** Constructor.
     */
    Framing (void);
    /** Destructor.
     */
    ~Framing (void);
    /** Render.
     * Renders the framing.
     */
    void Render (void);
private:
    union {
        struct {
            /** Vertex buffer.
             *  OpenGL buffer object storing the vertices of the framing.
             */
            GLuint vertexbuffer;
            /** Normal buffer.
             *  OpenGL buffer object storing the normals of the framing.
             */
            GLuint normalbuffer;
            /** Texture coordinate buffer.
             *  OpenGL buffer object storing the texture coordinates of the framing.
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
    /** Texture.
     * Texture object storing the texture used on the framing.
     */
    Texture texture;
};

#endif /* FRAMING_H */
