#version 430 core

const int GRID_WIDTH = 128;
const int GRID_HEIGHT = 64;
const int GRID_DEPTH = 128;
const uint NUM_PARTICLES = 32 * 32 * 32 * 2;

const vec3 GRID_SIZE = vec3 (GRID_WIDTH - 1, GRID_HEIGHT - 1, GRID_DEPTH - 1);

#define MAX_GRID_ID (GRID_WIDTH * GRID_HEIGHT * GRID_DEPTH - 1)

// parameters
const float rho_0 = 1.0;
const float h = 2.0;
const float epsilon = 10.0;
const float gravity = 10;
const float timestep = 0.016;

const float tensile_instability_k = 0.1;
const float tensile_instability_h = 0.2;

const float xsph_viscosity_c = 0.01;
const float vorticity_epsilon = 15;
