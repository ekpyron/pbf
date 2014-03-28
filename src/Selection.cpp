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
#include "Selection.h"

Selection::Selection (void)
{
	// load shaders
    selectionprogram.CompileShader (GL_VERTEX_SHADER, "shaders/selection/vertex.glsl");
    selectionprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/selection/fragment.glsl");
    selectionprogram.Link ();

    // create depth texture
    depthtexture.Bind (GL_TEXTURE_2D);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 1, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // create selection texture
    selectiontexture.Bind (GL_TEXTURE_2D);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_R32I, 1, 1, 0, GL_RED_INTEGER, GL_INT, NULL);

    // create framebuffer object
    glGenFramebuffers (1, framebuffers);

    // setup framebuffer
    glBindFramebuffer (GL_FRAMEBUFFER, selectionfb);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthtexture.get (), 0);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, selectiontexture.get (), 0);
    glBindFramebuffer (GL_FRAMEBUFFER, 0);
}

Selection::~Selection (void)
{
	// cleanup
	glDeleteFramebuffers (1, framebuffers);
}

GLint Selection::GetParticle (GLuint positionbuffer, GLuint numparticles, float xpos, float ypos)
{
	// setup viewport according to the specified position
	int width, height;
	glfwGetFramebufferSize (window, &width, &height);
	glBindFramebuffer (GL_FRAMEBUFFER, selectionfb);
	{
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers (1, buffers);
		const GLint v[] = { -1, -1, -1, -1 };
		glClearBufferiv (GL_COLOR, 0, &v[0]);
	}
    glViewport (-xpos, ypos - height, width, height);

    // pass the position buffer to the point sprite class
    pointsprite.SetPositionBuffer (positionbuffer, 4 * sizeof (float), 0);

    // clear depth buffer
   	glClear (GL_DEPTH_BUFFER_BIT);

   	//  render the spheres and write the particle id to the texture.
   	selectionprogram.Use ();
   	pointsprite.Render (numparticles);
	glBindFramebuffer (GL_FRAMEBUFFER, 0);

	// query and return the result
	selectiontexture.Bind (GL_TEXTURE_2D);
	GLint id;
	glGetTexImage (GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_INT, &id);
	return id;
}
