#include "Simulation.h"


Simulation::Simulation (void) : width (0), height (0), font ("textures/font.png"),
    last_fps_time (glfwGetTime ()), framecount (0), fps (0)
{
    // load shaders
    surroundingprogram.CompileShader (GL_VERTEX_SHADER, "shaders/surrounding/vertex.glsl");
    surroundingprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/surrounding/fragment.glsl");
    surroundingprogram.Link ();

    particleprogram.CompileShader (GL_VERTEX_SHADER, "shaders/particles/vertex.glsl");
    particleprogram.CompileShader (GL_FRAGMENT_SHADER, "shaders/particles/fragment.glsl");
    particleprogram.Link ();

    simulationstep.CompileShader (GL_COMPUTE_SHADER, "shaders/simulation/step.glsl");
    simulationstep.Link ();

    // create buffer objects
    glGenBuffers (7, buffers);

    // initialize the camera position and rotation and the transformation matrix buffer.
    camera.SetPosition (glm::vec3 (20, 10, 10));
    camera.Rotate (30.0f, 240.0f);
    glBindBufferBase (GL_UNIFORM_BUFFER, 0, transformationbuffer);
    glBufferData (GL_UNIFORM_BUFFER, sizeof (glm::mat4), glm::value_ptr (projmat * camera.GetViewMatrix()),
            GL_DYNAMIC_DRAW);


    // specify lighting parameters
    // and bind the uniform buffer object to binding point 1
    {
        struct lightparams {
            glm::vec3 lightpos;
            float padding;
            glm::vec3 spotdir;
            float spotexponent;
            float lightintensity;
        } lightparams;
        lightparams.lightpos = glm::vec3 (-20, 80, -2);
        lightparams.spotdir = glm::normalize (glm::vec3 (0.25, -1, 0.25));
        lightparams.spotexponent = 8.0f;
        lightparams.lightintensity = 10000.0f;
        glBindBufferBase (GL_UNIFORM_BUFFER, 1, lightingbuffer);
        glBufferData (GL_UNIFORM_BUFFER, sizeof (lightparams), &lightparams, GL_STATIC_DRAW);
    }

    // enable depth testing
    glEnable (GL_DEPTH_TEST);

    // enable back face culling
    glEnable (GL_CULL_FACE);
    glFrontFace (GL_CCW);

    // set color and depth for the glClear call
    glClearColor (0.25f, 0.25f, 0.25f, 1.0f);
    glClearDepth (1.0f);

    //  allocate particle buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, particlebuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (particleinfo_t) * GetNumberOfParticles (), NULL, GL_DYNAMIC_DRAW);

    // Initialize particle buffer
    ResetParticleBuffer ();

    // allocate lambda buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, lambdabuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * GetNumberOfParticles (), NULL, GL_DYNAMIC_DRAW);


    // allocate auxillary buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, auxbuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * 4 * GetNumberOfParticles (), NULL, GL_DYNAMIC_DRAW);
    // clear auxiliary buffer
    const float auxdata[] = { 0.25, 0, 1, 1 };
    glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, &auxdata[0]);

    // allocate the grid counter buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, gridcounterbuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (GLuint) * 128 * 16 * 128, NULL, GL_DYNAMIC_DRAW);

    // allocate grid cell buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, gridcellbuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (GLuint) * 128 * 16 * 128 * 64, NULL, GL_DYNAMIC_DRAW);

    // pass the position buffer to the icosahedron class
    icosahedron.SetPositionBuffer (particlebuffer, sizeof (particleinfo_t), 0);

    // pass the auxiliary buffer as color buffer to the icosahedron class
    icosahedron.SetColorBuffer (auxbuffer, sizeof (float) * 4, 0);

    // initialize last frame time
    last_time = glfwGetTime ();
}

Simulation::~Simulation (void)
{
    // cleanup
    glDeleteBuffers (7, buffers);
}

void Simulation::Resize (const unsigned int &_width, const unsigned int &_height)
{
    // update the stored framebuffer dimensions
    width = _width;
    height = _height;

    // update the stored projection matrix and pass it to the render program
    projmat = glm::perspective (45.0f * float (M_PI / 180.0f), float (width) / float (height), 0.1f, 1000.0f);
    glBindBuffer (GL_UNIFORM_BUFFER, transformationbuffer);
    glBufferSubData (GL_UNIFORM_BUFFER, 0, sizeof (glm::mat4), glm::value_ptr (projmat * camera.GetViewMatrix ()));
}

void Simulation::OnMouseMove (const double &x, const double &y)
{
    // handle mouse events and pass them to the camera class
    if (glfwGetMouseButton (window, GLFW_MOUSE_BUTTON_LEFT))
    {
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

        // update the view matrix
        glBindBuffer (GL_UNIFORM_BUFFER, transformationbuffer);
        glBufferSubData (GL_UNIFORM_BUFFER, 0, sizeof (glm::mat4),
                glm::value_ptr (projmat * camera.GetViewMatrix ()));
    }
}

const unsigned int Simulation::GetNumberOfParticles (void) const
{
    return 32 * 32 * 8 * 2;
}

void Simulation::ResetParticleBuffer (void)
{
    // regenerate particle information
    std::vector<particleinfo_t> particles;
    particles.reserve (GetNumberOfParticles ());
    srand (time (NULL));
    for (int x = 0; x < 32; x++)
    {
        for (int z = 0; z < 32; z++)
        {
            for (int y = 0; y < 8; y++)
            {
                particleinfo_t particle;
                particle.position = glm::vec3 (32.5 + x, y + 0.5, 32.5 + z);
/*                particle.position += 0.01f * glm::vec3 (float (rand ()) / float (RAND_MAX) - 0.5f,
                        float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f);*/
                /*particle.velocity = glm::vec3 (float (rand ()) / float (RAND_MAX) - 0.5f,
                        float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f);*/
                particles.push_back (particle);
            }
        }
    }
    for (int x = 0; x < 32; x++)
    {
        for (int z = 0; z < 32; z++)
        {
            for (int y = 0; y < 8; y++)
            {
                particleinfo_t particle;
                particle.position = glm::vec3 (32.5 + 63 - x, y + 0.5, 32.5 + 63 - z);
/*                particle.position += 0.01f * glm::vec3 (float (rand ()) / float (RAND_MAX) - 0.5f,
                        float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f);*/
                /*particle.velocity = glm::vec3 (float (rand ()) / float (RAND_MAX) - 0.5f,
                        float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f);*/
                particles.push_back (particle);
            }
        }
    }
    //  update the particle buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, particlebuffer);
    glBufferSubData (GL_SHADER_STORAGE_BUFFER, 0, sizeof (particleinfo_t) * particles.size (), &particles[0]);
    // clear auxiliary buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, auxbuffer);
    const float auxdata[] = { 0.25, 0, 1, 1 };
    glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, &auxdata[0]);
}

void Simulation::OnKeyUp (int key)
{
    switch (key)
    {
    case GLFW_KEY_TAB:
        ResetParticleBuffer ();
        break;
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

    // render the framing
    surroundingprogram.Use ();
    framing.Render ();

    // clear grid counter buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, gridcounterbuffer);
    glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, NULL);

    // clear lambda buffer
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, lambdabuffer);
    glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, NULL);

    // run simulation step 1
    if (glfwGetKey (window, GLFW_KEY_SPACE))
    {
        // clear auxiliary buffer
        glBindBuffer (GL_SHADER_STORAGE_BUFFER, auxbuffer);
        const float auxdata[] = { 0.25, 0, 1, 1 };
        glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, &auxdata[0]);

        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, particlebuffer);
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, gridcounterbuffer);
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 2, gridcellbuffer);
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 3, lambdabuffer);
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 4, auxbuffer);

        simulationstep.Use ();
        glMemoryBarrier (GL_BUFFER_UPDATE_BARRIER_BIT);
        glDispatchCompute (16, 16, 1);
        err = glGetError ();
        if (err != GL_NO_ERROR)
        {
            std::cerr << "OpenGL Error Detected: 0x" << std::hex << err << std::endl;
            return false;
        }
        glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // render spheres
    particleprogram.Use ();
    icosahedron.Render (GetNumberOfParticles ());

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
    }
    return true;
}
