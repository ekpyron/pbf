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
     * \param specifies the texture target
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
