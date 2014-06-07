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
#ifndef FRAMINGMESH_H
#define FRAMINGMESH_H

#include "common.h"
#include "Texture.h"
#include "ShaderProgram.h"
#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cfileio.h>

/** Scene framing.
 * This class renders a framing for the scene.
 */
class FramingMesh
{
public:
    /** Constructor.
     */
    FramingMesh (const char* input);
    /** Destructor.
     */
    ~FramingMesh (void);
    /** Render.
     * Renders the framing.
     */
    void Render (void);
	
    void LoadMesh (const std::string &fileName);
private:
    /** Shader program.
     * Shader program for displaying the surrounding scene.
    */
    ShaderProgram program;

    /** Number of indices.
     * Number of indices in the mesh that are to be rendered.
     */
    GLuint num_indices;

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
            /** Index buffer.
             * OpenGL buffer object storing the vertex indices.
             */
            GLuint indexbuffer;
        };
        /** Buffer objects.
         * The buffer objects are stored in a union, so that it is possible
         * to create/delete all buffer objects with a single OpenGL call.
         */
        GLuint buffers[3];
    };
    /** Vertex array object.
     * OpenGL vertex array object used to store information about the layout and location
     * of the vertex attributes.
     */
    GLuint vertexarray;
};

#endif /* FRAMINGMESH_H */
