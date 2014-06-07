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
#include "common.h"
#include "Simulation.h"
#include "FullscreenQuad.h"
#include <stdlib.h>

/** \file main.cpp
 * Main source file.
 * The source file that contains the main entry point and performs initialization and cleanup.
 */

/** GLFW window.
 * The GLFW window.
 */
GLFWwindow *window = NULL;

/** OpenGL extension support flags.
 * Structure that contains flags indicating whether
 * specific OpenGL extensions are supported or not.
 */
glextflags_t GLEXTS;

/** Simulation class.
 * The global Simulation object.
 */
Simulation *simulation = NULL;

/** GLFW error callback.
 * Outputs error messages from GLFW.
 * \param error error id
 * \param msg error message
 */
void glfwErrorCallback (int error, const char *msg)
{
    std::cerr << "GLFW error: " << msg << std::endl;
}

/** OpenGL debug callback.
 * Outputs a debug message from OpenGL.
 * \param source source of the debug message
 * \param type type of the debug message
 * \param id debug message id
 * \param severity severity of the debug message
 * \param length length of the debug message
 * \param message debug message
 * \param userParam user pointer
 *
 */
void glDebugCallback (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
        const GLchar *message, const void *userParam)
{
    std::cerr << "OpenGL debug message: " << std::string (message, length) << std::endl;
}

/** Cursor position.
 * Stores the last known cursor position. Used to calculate relative cursor movement.
 */
glm::dvec2 cursor;

/** Mouse movement callback.
 * GLFW callback for mouse movement events. Passes the event to the Simulation class.
 * \param window GLFW window that produced the event
 * \param x new x coordinate of the cursor
 * \param y new y coordinate of the cursor
 */
void OnMouseMove (GLFWwindow *window, double x, double y)
{
    Simulation *simulation = reinterpret_cast<Simulation*> (glfwGetWindowUserPointer (window));
    simulation->OnMouseMove (x - cursor.x, y - cursor.y);
    cursor.x = x; cursor.y = y;
}

/** Mouse button callback.
 * GLFW callback for mouse button events. Passes the event to the Simulation class.
 * \param window GLFW window that produced the event
 * \param button the mouse button that was pressed or released
 * \param action one of GLFW_PRESS or GLFW_RELEASE
 * \param mode bit field describing which modifier keys were hold down
 */
void OnMouseButton (GLFWwindow *window, int button, int action, int mode)
{
    Simulation *simulation = reinterpret_cast<Simulation*> (glfwGetWindowUserPointer (window));
    if (action == GLFW_PRESS)
    	simulation->OnMouseDown (button);
    else if (action == GLFW_RELEASE)
    	simulation->OnMouseUp (button);
}

/** Key event callback.
 * GLFW callback for key events. Passes the event to the Simulation class.
 * \param window GLFW window that produced the event
 * \param key the key that was pressed or released
 * \param scancode system-specific scancode of the key
 * \param action GLFW_PRESS, GLFW_RELEASE or GLFW_REPEAT
 * \param mods Bit-field describing the modifier keys that were held down when the event ocurred
 */
void OnKeyEvent (GLFWwindow *window, int key, int scancode, int action, int mods)
{
    Simulation *simulation = reinterpret_cast<Simulation*> (glfwGetWindowUserPointer (window));
    if (action == GLFW_RELEASE)
        simulation->OnKeyUp (key);
    else if (action == GLFW_PRESS)
        simulation->OnKeyDown (key);
}

/** Window resize callback.
 * GLFW callback for window resize events. Passes the event to the Simulation class.
 * \param window GLFW window that was resized
 * \param width new window width
 * \param height new window height
 */
void OnResize (GLFWwindow *window, int width, int height)
{
    Simulation *simulation = reinterpret_cast<Simulation*> (glfwGetWindowUserPointer (window));
    simulation->Resize (width, height);
}

/** Broken ATI glMemoryBarrier entry point.
 * This stores the non functional glMemoryBarrier entry point provided by ATI drivers.
 */
PFNGLMEMORYBARRIERPROC _glMemoryBarrier_BROKEN_ATIHACK = 0;

/** Workaround for broken glMemoryBarrier provided by ATI drivers.
 * This is used as a hack to work around the broken implementation
 * of glMemoryBarrier provided by ATI drivers. It synchronizes
 * the GPU command queue using a sync object before calling the
 * apparently non functional entry point provided by the driver.
 * It is not clear whether this actually results in the desired
 * behavior, but it seems to produce much better results than
 * just calling glMemoryBarrier (which seems to have no effect
 * at all).
 * \param bitfield Specifies the barriers to insert.
 */
void _glMemoryBarrier_ATIHACK (GLbitfield bitfield)
{
	GLsync syncobj = glFenceSync (GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glWaitSync (syncobj, 0, GL_TIMEOUT_IGNORED);
	glDeleteSync (syncobj);
	_glMemoryBarrier_BROKEN_ATIHACK (bitfield);
}

/** Dummy override for glMemoryBarrier.
 * This is used as a dummy function that does nothing. glMemoryBarrier
 * will be overridden by this function, if the environment variable
 * PBF_NO_MEMORY_BARRIERS is set to 1.
 * \param bitfield ignored argument
 */
void _glMemoryBarrier_DISABLED (GLbitfield bitfield)
{
}

/** Legacy glBindBuffersBase.
 * Legacy implementation of glBindBuffersBase. This is used as a fallback if
 * GL_ARB_multi_bind is not available.
 * \param target Specify the target of the bind operation.
 * \param first Specify the index of the first binding point within the array specified by target.
 * \param count Specify the number of contiguous binding points to which to bind buffers.
 * \param buffers A pointer to an array of names of buffer objects to bind to the targets on the specified binding point, or NULL.
 */
void _glBindBuffersBase (GLenum target, GLuint first, GLsizei count, const GLuint *buffers)
{
	for (GLsizei i = 0; i < count; i++)
	{
		if (buffers == NULL)
			glBindBufferBase (target, first + i, 0);
		else
			glBindBufferBase (target, first + i, buffers[i]);
	}
}

/** Check for boolean environment setting.
 * Checks whether a environment variable is set and returns true,
 * if it is, unless its value starts with 0, f, F, n or N.
 * \param varname Environment variable to check.
 * \returns Whether the environment setting is set or not.
 */
bool CheckEnvironment (const char *varname)
{
	const char *env = getenv (varname);
	if (env != NULL) {
		switch (env[0])
		{
		case '0':
		case 'f':
		case 'F':
		case 'n':
		case 'N':
			return false;
		}
		return true;
	}
	return false;
}

/** Initialization.
 * Perform general initialization tasks.
 * \param mesh Collision mesh to be used.
 */
void initialize (const char *mesh)
{
	// check whether a debug context should be created
	bool debugcontext = !CheckEnvironment ("PBF_NO_DEBUG_CONTEXT");

    // set GLFW error callback
    glfwSetErrorCallback (glfwErrorCallback);

    // specify parameters for the opengl context
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint (GLFW_OPENGL_DEBUG_CONTEXT, debugcontext ? GL_TRUE : GL_FALSE);

    // open a window and an OpenGL context
    window = glfwCreateWindow (1280, 720, "PBF", NULL, NULL);
    if (window == NULL)
        throw std::runtime_error ("Cannot open window.");
    glfwMakeContextCurrent (window);

    // get OpenGL entry points
    glcorewInit ((glcorewGetProcAddressCallback) glfwGetProcAddress);

    // check for ATI card and enable workarounds
    std::string vendor (reinterpret_cast<const char*> (glGetString (GL_VENDOR)));
    if (!vendor.compare ("ATI Technologies Inc."))
    {
    	std::cout << "Enable ATI workarounds." << std::endl;
    	_glMemoryBarrier_BROKEN_ATIHACK = glMemoryBarrier;
    	glMemoryBarrier = _glMemoryBarrier_ATIHACK;
    }

    // check for environment variables and enable workarounds respectively
    if (CheckEnvironment ("PBF_NO_MEMORY_BARRIERS"))
    {
    	std::cout << "Disable glMemoryBarrier" << std::endl;
    	glMemoryBarrier = _glMemoryBarrier_DISABLED;
    }

    if (debugcontext)
    {
    	glDebugMessageCallback (glDebugCallback, NULL);
    	glEnable (GL_DEBUG_OUTPUT);
    }

    // determine OpenGL extension capabilities and apply workarounds where necessary
    GLEXTS.ARB_clear_texture = IsExtensionSupported ("GL_ARB_clear_texture");
    if (!IsExtensionSupported ("GL_ARB_multi_bind"))
    {
    	glBindBuffersBase = _glBindBuffersBase;
    }

    // create the simulation class
    simulation = new Simulation (mesh);

    // setup event callbacks
    glfwSetWindowUserPointer (window, simulation);
    glfwGetCursorPos (window, &cursor.x, &cursor.y);
    glfwSetCursorPosCallback (window, OnMouseMove);
    glfwSetMouseButtonCallback (window, OnMouseButton);
    glfwSetKeyCallback (window, OnKeyEvent);
    glfwSetFramebufferSizeCallback (window, OnResize);

    // notify the Simulation class of the initial window dimensions
    int width, height;
    glfwGetFramebufferSize (window, &width, &height);
    simulation->Resize (width, height);
}

/** Cleanup.
 */
void cleanup (void)
{
	// release simulation class
    if (simulation != NULL)
        delete simulation;
    // release signleton classes
    FullscreenQuad::Release ();
    // destroy window and shutdown glfw
    if (window != NULL)
        glfwDestroyWindow (window);
    glfwTerminate ();
}

bool IsExtensionSupported (const std::string &name)
{
	GLint n = 0;
	glGetIntegerv (GL_NUM_EXTENSIONS, &n);
	for (int i = 0; i < n; i++)
	{
		const char *ext = reinterpret_cast<const char*> (glGetStringi (GL_EXTENSIONS, i));
		if (ext != NULL && !name.compare (ext))
			return true;
	}
	return false;
}

char* getCmdOption(char** begin, char** end, const std::string& option)
{
	for (char **it = begin; it != end; it++)
	{
		if (!option.compare (*it) && it++ != end)
			return *it;
	}
	return 0;
}

/** Main.
 * Main entry point.
 * \param argc number of arguments
 * \param argv argument array
 * \returns error code
 */
int main (int argc, char *argv[])
{
    // initialize GLFW
    if (!glfwInit ())
    {
        std::cerr << "Cannot initialize GLFW." << std::endl;
        return -1;
    }
	// check if mesh argument was passed
	char* mesh = getCmdOption(argv, argv + argc, "-mesh");
    try {
        // initialization
        initialize (mesh);

        // simulation loop
        while (!glfwWindowShouldClose (window))
        {
            if (!simulation->Frame ())
                break;

            glfwSwapBuffers (window);
            glfwPollEvents ();
        }

        // cleanup
        cleanup ();
        return 0;
    } catch (std::exception &e) {
        cleanup ();
        std::cerr << "Exception: " << e.what () << std::endl;
        return -1;
    }
}
