#include "Selection.h"

Selection::Selection (void)
{
	// load shaders
    selectionprogram.CompileShader (GL_VERTEX_SHADER, "shaders/selection/vertex.glsl");
    selectionprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/selection/fragment.glsl",
    		"shaders/simulation/include.glsl");
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

GLint Selection::GetParticle (GLuint particlebuffer, GLuint numparticles, float xpos, float ypos)
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
    pointsprite.SetPositionBuffer (particlebuffer, sizeof (particleinfo_t), 0);

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
