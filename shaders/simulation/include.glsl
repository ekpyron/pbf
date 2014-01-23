#version 430 core

const int GRID_WIDTH = 128;
const int GRID_HEIGHT = 16;
const int GRID_DEPTH = 128;

const vec3 GRID_SIZE = vec3 (GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH);

#define MAX_GRID_ID (GRID_WIDTH * GRID_HEIGHT * GRID_DEPTH - 1)

// parameters
const float rho_0 = 0.85;
const float h = 2.0;
const float epsilon = 10;
const float gravity = 10;
const float timestep = 0.05;
const uint highlightparticle = 32 * 32 * 8 * 2 - 1;

