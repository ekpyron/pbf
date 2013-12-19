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
    glGenBuffers (6, buffers);

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

    // generate particle information
    typedef struct particleinfo {
        glm::vec3 position;
        float padding0;
        glm::vec3 oldposition;
        float padding2;
    } particleinfo_t;
    std::vector<particleinfo_t> particles;
    particles.reserve (32 * 32 * 8 * 2);
    srand (time (NULL));
    for (int x = 0; x < 32; x++)
    {
        for (int z = 0; z < 32; z++)
        {
            for (int y = 0; y < 8; y++)
            {
                particleinfo_t particle;
                particle.position = glm::vec3 (32 + x, y + 0, 32 + z);
                glm::vec3 velocity = glm::vec3 (float (rand ()) / float (RAND_MAX) - 0.5f,
                        float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f);
                particle.oldposition = particle.position - 0.01f * velocity;
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
                particle.position = glm::vec3 (32 + 63 - x, y + 0, 32 + 63 - z);
                glm::vec3 velocity = glm::vec3 (float (rand ()) / float (RAND_MAX) - 0.5f,
                        float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f);
                particle.oldposition = particle.position - 0.01f * velocity;
                particles.push_back (particle);
            }
        }
    }

    //  store the particle in a buffer object and bind it to shader storage buffer binding point 0
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, particlebuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (particleinfo_t) * particles.size (), &particles[0], GL_DYNAMIC_DRAW);

    // allocate lambda buffer and bind it to shader storage buffer binding point 3
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 3, lambdabuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * particles.size (), NULL, GL_DYNAMIC_DRAW);

    // allocate the grid counter buffer and bind it to shader storage buffer binding point 1
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, gridcounterbuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (GLuint) * 128 * 16 * 128, NULL, GL_DYNAMIC_DRAW);

    // allocate grid cell buffer and bind it to shader storage buffer binding point 2
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 2, gridcellbuffer);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (GLuint) * 128 * 16 * 128 * 8, NULL, GL_DYNAMIC_DRAW);

    // pass the position buffer to the icosahedron class1
    icosahedron.SetPositionBuffer (particlebuffer, sizeof (particleinfo_t), 0);

    // initialize last frame time
    last_time = glfwGetTime ();
}

Simulation::~Simulation (void)
{
    // cleanup
    glDeleteBuffers (6, buffers);
}

void Simulation::Resize (const unsigned int &_width, const unsigned int &_height)
{
    // update the stored framebuffer dimensions
    width = _width;
    height = _height;

    // update the stored projection matrix and pass it to the render program
    projmat = glm::perspective (45.0f, float (width) / float (height), 0.1f, 1000.0f);
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

void Simulation::OnKeyUp (int key)
{
    switch (key)
    {
    case GLFW_KEY_TAB:
    {
        // regenerate particle information
        typedef struct particleinfo {
            glm::vec3 position;
            float padding0;
            glm::vec3 oldposition;
            float padding2;
        } particleinfo_t;
        std::vector<particleinfo_t> particles;
        particles.reserve (32 * 32 * 8 * 2);
        srand (time (NULL));
        for (int x = 0; x < 32; x++)
        {
            for (int z = 0; z < 32; z++)
            {
                for (int y = 0; y < 8; y++)
                {
                    particleinfo_t particle;
                    particle.position = glm::vec3 (32 + x, y + 0, 32 + z);
                    glm::vec3 velocity = glm::vec3 (float (rand ()) / float (RAND_MAX) - 0.5f,
                            float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f);
                    particle.oldposition = particle.position - 0.01f * velocity;
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
                    particle.position = glm::vec3 (32 + 63 - x, y + 0, 32 + 63 - z);
                    glm::vec3 velocity = glm::vec3 (float (rand ()) / float (RAND_MAX) - 0.5f,
                            float (rand ()) / float (RAND_MAX) - 0.5f, float (rand ()) / float (RAND_MAX) - 0.5f);
                    particle.oldposition = particle.position - 0.01f * velocity;
                    particles.push_back (particle);
                }
            }
        }
        //  update the particle buffer
        glBindBuffer (GL_SHADER_STORAGE_BUFFER, particlebuffer);
        glBufferSubData (GL_SHADER_STORAGE_BUFFER, 0, sizeof (particleinfo_t) * particles.size (), &particles[0]);

        break;
    }
    }
}

void Simulation::Frame (void)
{
    float time_passed = glfwGetTime () - last_time;
    last_time += time_passed;

    // specify the viewport size
    glViewport (0, 0, width, height);

    // clear the color and depth buffer
    glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // render the framing
    surroundingprogram.Use ();
    framing.Render ();

    // clear grid counter buffer
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, gridcounterbuffer);
    glClearBufferData (GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, NULL);

    // run simulation step 1
    if (glfwGetKey (window, GLFW_KEY_SPACE))
    {
        simulationstep.Use ();
        glMemoryBarrier (GL_BUFFER_UPDATE_BARRIER_BIT);
        glDispatchCompute (16, 4, 1);
        glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // render spheres
    particleprogram.Use ();
    icosahedron.Render (32 * 32 * 8 * 2);

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
}
