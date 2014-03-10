#include "Simulation.h"


Simulation::Simulation (void) : width (0), height (0), font ("textures/font.png"),
    last_fps_time (glfwGetTime ()), framecount (0), fps (0), running (false),
    usesurfacereconstruction (false), sph (GetNumberOfParticles ()), useskybox (false),
    envmap (NULL), usenoise (false), guitimer (0.0f), guistate (GUISTATE_REST_DENSITY)
{
	// load shaders
    particleprogram.CompileShader (GL_VERTEX_SHADER, "shaders/particles/vertex.glsl");
    particleprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/particles/fragment.glsl");
    particleprogram.Link ();

    // create buffer objects
    glGenBuffers (2, buffers);

    // create query objects
    glGenQueries (1, &renderingquery);

    // initialize the camera position and rotation and the transformation matrix buffer.
    camera.SetPosition (glm::vec3 (20, 10, 10));
    camera.Rotate (30.0f, 240.0f);
    glBindBuffer (GL_UNIFORM_BUFFER, transformationbuffer);
    glBufferData (GL_UNIFORM_BUFFER, sizeof (transformationbuffer_t), NULL, GL_DYNAMIC_DRAW);

    // specify lighting parameters
    // and bind the uniform buffer object to binding point 1
    {
        lightparams_t lightparams;
        lightparams.lightpos = glm::vec3 (-20, 80, -2);
        lightparams.spotdir = glm::normalize (glm::vec3 (0.25, -1, 0.25));
        lightparams.spotexponent = 8.0f;
        lightparams.lightintensity = 10000.0f;
        glBindBuffer (GL_UNIFORM_BUFFER, lightingbuffer);
        glBufferData (GL_UNIFORM_BUFFER, sizeof (lightparams), &lightparams, GL_STATIC_DRAW);
    }

    // bind uniform buffers to their correct binding points
    glBindBufferBase (GL_UNIFORM_BUFFER, 0, transformationbuffer);
    glBindBufferBase (GL_UNIFORM_BUFFER, 1, lightingbuffer);

    // enable depth testing
    glEnable (GL_DEPTH_TEST);

    // enable back face culling
    glEnable (GL_CULL_FACE);
    glFrontFace (GL_CCW);

    // set color and depth for the glClear call
    glClearColor (0.25f, 0.25f, 0.25f, 1.0f);
    glClearDepth (1.0f);

    // Initialize particle buffer
    ResetParticleBuffer ();

    // pass position and color to the point sprite class
    pointsprite.SetPositionBuffer (sph.GetPositionBuffer (), 4 * sizeof (float), 0);
    pointsprite.SetHighlightBuffer (sph.GetHighlightBuffer (), sizeof (GLuint), 0);

    // update view matrix
    UpdateViewMatrix ();

    // initialize last frame time
    last_time = glfwGetTime ();
}

Simulation::~Simulation (void)
{
    // cleanup
	if (envmap) delete envmap;
	glDeleteQueries (1, &renderingquery);
    glDeleteBuffers (2, buffers);
}

void Simulation::Resize (const unsigned int &_width, const unsigned int &_height)
{
    // update the stored framebuffer dimensions
    width = _width;
    height = _height;

    // update the stored projection matrix and pass it to the render program
    projmat = glm::perspective (45.0f * float (M_PI / 180.0f), float (width) / float (height), 1.0f, 200.0f);

    glBindBuffer (GL_UNIFORM_BUFFER, transformationbuffer);
    glBufferSubData (GL_UNIFORM_BUFFER, offsetof (transformationbuffer_t, projmat),
    		sizeof (glm::mat4), glm::value_ptr (projmat));
}

void Simulation::UpdateViewMatrix (void)
{
    // update the view matrix
    glBindBuffer (GL_UNIFORM_BUFFER, transformationbuffer);
    glBufferSubData (GL_UNIFORM_BUFFER, offsetof (transformationbuffer_t, viewmat),
    		sizeof (glm::mat4), glm::value_ptr (camera.GetViewMatrix ()));
    glBufferSubData (GL_UNIFORM_BUFFER, offsetof (transformationbuffer_t, invviewmat),
    		sizeof (glm::mat4), glm::value_ptr (glm::inverse (camera.GetViewMatrix ())));

    // update eye position
    glm::vec3 eyepos = camera.GetPosition ();
    glBindBuffer (GL_UNIFORM_BUFFER, lightingbuffer);
    glBufferSubData (GL_UNIFORM_BUFFER, offsetof (lightparams_t, eyepos), sizeof (eyepos), glm::value_ptr (eyepos));
}

void Simulation::OnMouseMove (const double &x, const double &y)
{
    // handle mouse events and pass them to the camera class
    if (glfwGetMouseButton (window, GLFW_MOUSE_BUTTON_LEFT))
    {
    	// ignore mouse movement, when in highlighting mode
    	if (glfwGetKey (window, GLFW_KEY_H)) return;

        if (glfwGetKey (window, GLFW_KEY_LEFT_CONTROL))
        {
            camera.Zoom (x + y);
        }
        else if (glfwGetKey (window, GLFW_KEY_LEFT_SHIFT))
        {
            camera.MoveX (x);
            camera.MoveY (y);
        }
        else
        {
            camera.Rotate (y, -x);
        }
        UpdateViewMatrix ();
    }
}

void Simulation::OnMouseUp (const int &button)
{
}

void Simulation::OnMouseDown (const int &button)
{
	if (glfwGetKey (window, GLFW_KEY_H))
	{
		double xpos, ypos;
		glfwGetCursorPos (window, &xpos, &ypos);
		GLint id = selection.GetParticle (sph.GetPositionBuffer (), GetNumberOfParticles (), xpos, ypos);
		if (id >= 0)
		{
			// create a temporary buffer
			GLuint tmpbuffer;
			glGenBuffers (1, &tmpbuffer);
			glBindBuffer (GL_COPY_WRITE_BUFFER, tmpbuffer);
			glBufferData (GL_COPY_WRITE_BUFFER, sizeof (GLuint), NULL, GL_DYNAMIC_READ);

			// copy the particle highlighting information to the temporary buffer
			// a temporary buffer is used, so that drivers (in particular the NVIDIA driver)
			// are not tempted to move the whole buffer from GPU to host/DMA memory
			glBindBuffer (GL_COPY_READ_BUFFER, sph.GetHighlightBuffer ());
			glCopyBufferSubData (GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
					id * sizeof (GLuint), 0, sizeof (GLuint));

			// map the temporary buffer to CPU address space
			GLuint *info = reinterpret_cast<GLuint*> (glMapBuffer (GL_COPY_WRITE_BUFFER, GL_READ_WRITE));
			if (info == NULL)
				throw std::runtime_error ("A GPU buffer could not be mapped to CPU address space.");

			// modify the particle information i order to highlight/unhighlight the particle
			if (*info > 0)
				*info = 0;
			else
				*info = 1;

			// unmap the temporary buffer
			glUnmapBuffer (GL_COPY_WRITE_BUFFER);

			// copy back to the highlighting buffer
			glCopyBufferSubData (GL_COPY_WRITE_BUFFER, GL_COPY_READ_BUFFER,
					0, id * sizeof (GLuint), sizeof (GLuint));

			// delete the temporary buffer
			glDeleteBuffers (1, &tmpbuffer);
		}
	}
}

const unsigned int Simulation::GetNumberOfParticles (void) const
{
	// must be a multiple of 512
    return 32 * 32 * 32 * 2;
}

void Simulation::ResetParticleBuffer (void)
{
    // regenerate particle information
	std::vector<glm::vec4> positions;
	std::vector<glm::vec4> velocities;

    const float scale = 0.94f;
    int id = 0;

    srand (time (NULL));
    for (int x = 0; x < 32; x++)
    {
        for (int z = 0; z < 32; z++)
        {
            for (int y = 0; y < 32; y++)
            {
                glm::vec4 position = glm::vec4 (32.5f, 0.5f, 32.5f, 0.0f) + scale * glm::vec4 (x, y, z, 0.0f);
                position += 0.01f * glm::vec4 (float (rand ()) / float (RAND_MAX) - 0.5f,
                		float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f, 0);
                glm::vec4 velocity = glm::vec4 (0, 0, 0, 0);
                positions.push_back (position);
                velocities.push_back (velocity);
            }
        }
    }
    for (int x = 0; x < 32; x++)
    {
        for (int z = 0; z < 32; z++)
        {
            for (int y = 0; y < 32; y++)
            {
                glm::vec4 position = glm::vec4 (32.5f + 63.0f, 0.5f, 32.5f + 63.0f, 0.0f)
                	+ scale * glm::vec4 (-x, y, -z, 0.0f);
                position += 0.01f * glm::vec4 (float (rand ()) / float (RAND_MAX) - 0.5f,
                		float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f, 0);
                glm::vec4 velocity = glm::vec4 (0, 0, 0, 0);
                positions.push_back (position);
                velocities.push_back (velocity);
            }
        }
    }

    // create temporary buffer
    GLuint tmpbuffer;
    glGenBuffers (1, &tmpbuffer);

    // upload position data
    glBindBuffer (GL_COPY_READ_BUFFER, tmpbuffer);
    glBufferData (GL_COPY_READ_BUFFER, 4 * sizeof (float) * positions.size (), &positions[0], GL_STREAM_COPY);

    // copy the new position data to SPH position buffer
    glBindBuffer (GL_COPY_WRITE_BUFFER, sph.GetPositionBuffer ());
    glCopyBufferSubData (GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, 4 * sizeof (float) * positions.size ());

    // upload velocity data
    glBufferData (GL_COPY_READ_BUFFER, 4 * sizeof (float) * velocities.size (), &velocities[0], GL_STREAM_COPY);

    // copy the new velocity data to SPH position buffer
    glBindBuffer (GL_COPY_WRITE_BUFFER, sph.GetVelocityBuffer ());
    glCopyBufferSubData (GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, 4 * sizeof (float) * velocities.size ());

    // delete temporary buffer
    glDeleteBuffers (1, &tmpbuffer);

    // clear highlight buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, sph.GetHighlightBuffer ());
    glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED, GL_UNSIGNED_INT, NULL);
}

void Simulation::OnKeyDown (int key)
{
	switch (key)
	{
	// enable external force
	case GLFW_KEY_F:
		sph.SetExternalForce (true);
		break;
	}
}

void Simulation::OnKeyUp (int key)
{
    switch (key)
    {
    // toggle surface reconstruction
    case GLFW_KEY_ENTER:
    	usesurfacereconstruction = !usesurfacereconstruction;
    	break;
    // toggle simulation
    case GLFW_KEY_SPACE:
    	running = !running;
    	break;
    // toggle vorticity confinement
    case GLFW_KEY_V:
    	sph.SetVorticityConfinementEnabled (!sph.IsVorticityConfinementEnabled ());
    	break;
    // reset to initial particle configuration
    case GLFW_KEY_TAB:
        ResetParticleBuffer ();
        break;
    // output the queried time frames spent in the
    // different simulation stages
    case GLFW_KEY_T:
    {
    	sph.OutputTiming ();
    	if (glIsQuery (renderingquery))
    	{
    		GLint64 v;
    		glGetQueryObjecti64v (renderingquery, GL_QUERY_RESULT, &v);
    		std::cout << "Rendering: " << double (v) / 1000000.0 << " ms" << std::endl;
    	}
    	break;
    }
    // toggle environment map
    case GLFW_KEY_E:
    	useskybox = !useskybox;
    	if (useskybox)
    	{
    		if (envmap == NULL)
    		{
    			// load environment map
    			envmap = new Texture ();
    			envmap->Bind (GL_TEXTURE_CUBE_MAP);
    			Texture::Load (GL_TEXTURE_CUBE_MAP_POSITIVE_X, "textures/sky/skybox_posx.png", GL_COMPRESSED_RGB);
    			Texture::Load (GL_TEXTURE_CUBE_MAP_NEGATIVE_X, "textures/sky/skybox_negx.png", GL_COMPRESSED_RGB);
    			Texture::Load (GL_TEXTURE_CUBE_MAP_POSITIVE_Y, "textures/sky/skybox_posy.png", GL_COMPRESSED_RGB);
    			Texture::Load (GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, "textures/sky/skybox_negy.png", GL_COMPRESSED_RGB);
    			Texture::Load (GL_TEXTURE_CUBE_MAP_POSITIVE_Z, "textures/sky/skybox_posz.png", GL_COMPRESSED_RGB);
    			Texture::Load (GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, "textures/sky/skybox_negz.png", GL_COMPRESSED_RGB);
    			glGenerateMipmap (GL_TEXTURE_CUBE_MAP);
    			glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    			glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    			glEnable (GL_TEXTURE_CUBE_MAP_SEAMLESS);
    		}
    		surfacereconstruction.SetEnvironmentMap (envmap);
    	}
    	else
    	{
    		surfacereconstruction.SetEnvironmentMap (NULL);
    	}
    	break;
    // single simulation step
    case GLFW_KEY_S:
    	sph.Run ();
    	break;
    // toggle noise
    case GLFW_KEY_N:
    	usenoise = !usenoise;
    	if (usenoise)
    		surfacereconstruction.EnableNoise ();
    	else
    		surfacereconstruction.DisableNoise ();
    	break;
    // disable external force
    case GLFW_KEY_F:
    	sph.SetExternalForce (false);
    	break;
    case GLFW_KEY_RIGHT:
    	guistate = (guistate_t) ((int (guistate) + 1) % GUISTATE_NUM_STATES);
    	guitimer = 5.0f;
    	break;
    case GLFW_KEY_LEFT:
    	guistate = (guistate_t) ((int (guistate) + GUISTATE_NUM_STATES - 1) % GUISTATE_NUM_STATES);
    	guitimer = 5.0f;
    	break;
    }
    float factor = 1;
    if (glfwGetKey (window, GLFW_KEY_LEFT_SHIFT))
    	factor *= 10;
    if (glfwGetKey (window, GLFW_KEY_LEFT_CONTROL))
    	factor *= 10;
    switch (key)
    {
    case GLFW_KEY_DOWN:
    case GLFW_KEY_KP_SUBTRACT:
    case '-':
    	factor *= -1;
    case GLFW_KEY_UP:
    case GLFW_KEY_KP_ADD:
    case '+':
    	guitimer = 5.0f;
    	switch (guistate)
    	{
    	case GUISTATE_REST_DENSITY:
    		sph.SetRestDensity (glm::max (sph.GetRestDensity () + factor * 0.01, 0.001));
    		break;
    	case GUISTATE_CFM_EPSILON:
    		sph.SetCFMEpsilon (glm::max (sph.GetCFMEpsilon () + factor, 0.01f));
    		break;
    	case GUISTATE_GRAVITY:
    		sph.SetGravity (sph.GetGravity () + factor);
    		break;
    	case GUISTATE_TIMESTEP:
    		sph.SetTimestep (glm::min (sph.GetTimestep () + 0.001 * factor, 0.001));
    		break;
    	case GUISTATE_TENSILE_INSTABILITY_K:
    		sph.SetTensileInstabilityK (glm::min (sph.GetTensileInstabilityK () + factor * 0.1, 0.1));
    		break;
    	case GUISTATE_TENSILE_INSTABILITY_SCALE:
    		sph.SetTensileInstabilityScale (glm::min (sph.GetTensileInstabilityScale () + factor * 0.1, 0.1));
    		break;
    	case GUISTATE_XSPH_VISCOSITY:
    		sph.SetXSPHViscosity (glm::min (sph.GetXSPHViscosity () + factor * 0.01, 0.01));
    		break;
    	case GUISTATE_VORTICITY_EPSILON:
    		sph.SetVorticityEpsilon (glm::min (sph.GetVorticityEpsilon () + factor * 0.1, 0.1));
    		break;
    	}
    	break;
    }

    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12)
    {
    	key -= GLFW_KEY_F1;
    	key %= GUISTATE_NUM_STATES;
    	guistate = (guistate_t) key;
    	guitimer = 5.0f;
    }
}

bool Simulation::Frame (void)
{
    float time_passed = glfwGetTime () - last_time;
    last_time += time_passed;

    GLenum err = glGetError ();
    if (err != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error detected at frame begin: 0x" << std::hex << err << std::endl;
        return false;
    }

    // specify the viewport size
    glViewport (0, 0, width, height);

    // clear the color and depth buffer
    glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // render sky box
    if (useskybox && envmap != NULL)
    {
    	glDisable (GL_DEPTH_TEST);
    	skybox.Render (*envmap);
    	glEnable (GL_DEPTH_TEST);
    }

    // render the framing
    framing.Render ();

    // run simulation step 1
    if (running)
    	sph.Run ();

	glBeginQuery (GL_TIME_ELAPSED, renderingquery);
    if (!usesurfacereconstruction)
    {
    	// render icosahedra/spheres
    	particleprogram.Use ();
        // pass the position buffer to the point sprite class
        pointsprite.SetPositionBuffer (sph.GetPositionBuffer (), 4 * sizeof (float), 0);
    	pointsprite.Render (GetNumberOfParticles ());
    }
    else
    {
    	// render reconstructed surface
    	surfacereconstruction.Render (sph.GetPositionBuffer (), GetNumberOfParticles (), width, height);
    }
	glEndQuery (GL_TIME_ELAPSED);

    // determine the framerate every second
    framecount++;
    if (glfwGetTime () >= last_fps_time + 1)
    {
        fps = framecount;
        framecount = 0;
        last_fps_time = glfwGetTime ();
    }
    // display the framerate
    {
        std::stringstream stream;
        stream << "FPS: " << fps << std::endl;
        font.PrintStr (0, 0, stream.str ());

        if (guitimer > 0)
        {
        	guitimer -= time_passed;
        	std::stringstream stream;
        	switch (guistate)
        	{
        	case GUISTATE_REST_DENSITY:
        		stream << "Rest density: " << sph.GetRestDensity ();
        		break;
        	case GUISTATE_CFM_EPSILON:
        		stream << "CFM epsilon: " << sph.GetCFMEpsilon ();
        		break;
        	case GUISTATE_GRAVITY:
        		stream << "Gravity: " << sph.GetGravity ();
        		break;
        	case GUISTATE_TIMESTEP:
        		stream << "Timestep: " << sph.GetTimestep ();
        		break;
        	case GUISTATE_TENSILE_INSTABILITY_K:
        		stream << "Tensile instability k: " << sph.GetTensileInstabilityK ();
        		break;
        	case GUISTATE_TENSILE_INSTABILITY_SCALE:
        		stream << "Tensile instability scale: " << sph.GetTensileInstabilityScale ();
        		break;
        	case GUISTATE_XSPH_VISCOSITY:
        		stream << "XSPH viscosity: " << sph.GetXSPHViscosity ();
        		break;
        	case GUISTATE_VORTICITY_EPSILON:
        		stream << "Vorticity epsilon: " << sph.GetVorticityEpsilon ();
        		break;

        	}
    		font.PrintStr (39.0f - float (stream.str ().size ()) / 2.0f, 0, stream.str ());
        }
    }
    return true;
}
