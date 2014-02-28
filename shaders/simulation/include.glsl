#version 430 core

const vec3 GRID_SIZE = vec3 (128, 64, 128);
const ivec3 GRID_HASHWEIGHTS = ivec3 (1, 128 * 128, 128);

// parameters
const float rho_0 = 1.0;
const float h = 2.0;
const float epsilon = 5.0;
const float gravity = 10;
const float timestep = 0.016;

const float tensile_instability_k = 0.1;
const float tensile_instability_h = 0.2;

const float xsph_viscosity_c = 0.01;
const float vorticity_epsilon = 5;
