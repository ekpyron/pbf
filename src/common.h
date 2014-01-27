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

/** Particle information.
 * Structure representing the information stored for each particle.
 */
typedef struct particleinfo {
    /** Particle position.
     */
    glm::vec3 position;
    /** Unused padding value.
     */
    float padding0;
    /** Old particle position.
     */
    glm::vec3 oldposition;
    /** Unused padding value.
     */
    float padding2;
} particleinfo_t;

#endif /* !defined COMMON_H */
