#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_shader_subgroup_basic : enable

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

struct ParticleData {
    vec3 position;
    float aux;
};

layout(std430, binding = 0, set = 1) readonly buffer Particles
{
    ParticleData particles[];
};

struct GridExtents {
    int startIndex;
    int endIndex;
};

layout(std430, binding = 0, set = 0) readonly buffer GridData
{
    GridExtents grid[]; // .x: start; .y: end
};


layout(std140, binding = 2, set = 1) uniform GridData {
    ivec3 GRID_MAX;
    ivec3 GRID_MIN;
    ivec3 GRID_HASHWEIGHTS;
};

uint GetGridIndex(vec3 position) {
    highp ivec3 grid = ivec3 (clamp (position, GRID_MIN, GRID_MAX));
    return uint (dot (grid - GRID_MIN, GRID_HASHWEIGHTS));
}


void main (void)
{
    uint gid;
    gid = gl_GlobalInvocationID.x;

    if (gid == 0)
    {
        grid[0].startIndex = 0;
        uint maxID = gl_NumWorkGroups.x * gl_WorkGroupSize.x;
        grid[GetGridIndex(GRID_MAX)].endIndex = maxID;
        return;
    }

    uint currentGridIndex = GetGridIndex(particled[gid].position);
    uint previousGridIndex = GetGridIndex(particled[gid - 1].position);

    if (currentGridIndex != previousGridIndex)
    {
        grid[currentGridIndex].startIndex = gid;
        grid[previousGridIndex].endIndex = gid;
    }
}
