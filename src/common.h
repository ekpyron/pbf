#ifndef COMMON_H
#define COMMON_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <glcorew.h>
#define GLM_SWIZZLE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/noise.hpp>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <cmath>
#include <time.h>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923
#endif

extern GLFWwindow *window;

/** Particle key.
 * Structure representing key information for each particle.
 */
typedef struct particlekey {
	/** Particle position.
	 */
	glm::vec3 position;
	/** Particle ID.
	 */
	GLuint id;
} particlekey_t;

/** Check for OpenGL extension.
 * Checks whether an OpenGL extension is supported.
 * \param name Name of the extension.
 * \returns True, if the extension is supported, false, if not.
 */
bool IsExtensionSupported (const std::string &name);

#endif /* !defined COMMON_H */
