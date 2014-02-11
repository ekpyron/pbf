// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

layout (std430, binding = 0) buffer ParticleKeys
{
	ParticleKey particlekeys[];
};

layout (std430, binding = 1) buffer ParticleBuffer
{
	ParticleInfo particles[];
};

layout (std430, binding = 3) coherent buffer VorticityBuffer
{
	float vorticities[];
};

layout (binding = 0) uniform isamplerBuffer neighbourcelltexture;

float Wpoly6 (float r)
{
	if (r > h)
		return 0;
	float tmp = h * h - r * r;
	return 1.56668147106 * tmp * tmp * tmp / (h*h*h*h*h*h*h*h*h);
}

vec3 gradWspiky (vec3 r)
{
	float l = length (r);
	if (l > h || l == 0)
		return vec3 (0, 0, 0);
	float tmp = h - l;
	return (-3 * 4.774648292756860 * tmp * tmp) * r / (l * h*h*h*h*h*h);
}

#define FOR_EACH_NEIGHBOUR(var) for (int o = 0; o < 3; o++) {\
		ivec3 datav = texelFetch (neighbourcelltexture, int (gl_GlobalInvocationID.x * 3 + o)).xyz;\
		for (int comp = 0; comp < 3; comp++) {\
		int data = datav[comp];\
		int entries = data >> 24;\
		data = data & 0xFFFFFF;\
		if (data == 0) continue;\
		for (int var = data; var < data + entries; var++) {\
		if (var != gl_GlobalInvocationID.x) {
#define END_FOR_EACH_NEIGHBOUR(var)	}}}}

void main (void)
{
	uint particleid = particlekeys[gl_GlobalInvocationID.x].id;
	vec3 position = particlekeys[gl_GlobalInvocationID.x].position;

	// fetch velocity	
	vec3 velocity = particles[particleid].velocity;
	
	// calculate vorticity & apply XSPH viscosity
	vec3 v = vec3 (0, 0, 0);
	vec3 vorticity = vec3 (0, 0, 0);
	float rho = 0;
	FOR_EACH_NEIGHBOUR(j)
	{
		ParticleKey key_j = particlekeys[j];
		vec3 v_ij = particles[key_j.id].velocity - velocity;
		vec3 p_ij = position - key_j.position;
		float tmp = Wpoly6 (length (p_ij));
		rho += tmp;
		v += v_ij * tmp;
		vorticity += cross (v_ij, gradWspiky (p_ij));
	}
	END_FOR_EACH_NEIGHBOUR(j)
	velocity += xsph_viscosity_c * v;
	
	vorticities[particleid] = length (vorticity);
	
	barrier ();
	memoryBarrierBuffer ();
	
	// vorticity confinement
	vec3 gradVorticity = vec3 (0, 0, 0);
	FOR_EACH_NEIGHBOUR(j)
	{
		vec3 p_ij = position - particlekeys[j].position;
		gradVorticity += vorticities[j] * gradWspiky (p_ij);
	}
	END_FOR_EACH_NEIGHBOUR(j)
	
	float l = length (gradVorticity);
	if (l > 0)
		gradVorticity /= l;
	vec3 N = gradVorticity;
	
	// apply vorticity force
	velocity += timestep * vorticity_epsilon * cross (N, vorticity);
	
	// update particle information
	particles[particleid].velocity = velocity;
}
