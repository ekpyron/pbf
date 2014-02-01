// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

struct ParticleInfo
{
	vec3 position;
	float vorticity;
	vec3 oldposition;
	float highlighted;
};

layout (std430, binding = 0) buffer ParticleBuffer
{
	ParticleInfo particles[];
};

layout (std430, binding = 1) writeonly buffer AuxBuffer
{
	vec4 auxdata[];
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
		ivec3 datav = texelFetch (neighbourcelltexture, int (particleid * 3 + o)).xyz;\
		for (int comp = 0; comp < 3; comp++) {\
		int data = datav[comp];\
		int entries = data >> 24;\
		data = data & 0xFFFFFF;\
		if (data == 0) continue;\
		for (int var = data; var < data + entries; var++) {\
		if (var != particleid) {
#define END_FOR_EACH_NEIGHBOUR(var)	}}}}

void main (void)
{
	uint particleid;
	particleid = gl_GlobalInvocationID.x;

	ParticleInfo particle = particles[particleid];
	
	// calculate velocity	
	vec3 velocity = (particle.position - particle.oldposition) / timestep;

	// vorticity confinement & XSPH viscosity
	vec3 v = vec3 (0, 0, 0);
	vec3 vorticity = vec3 (0, 0, 0);
	FOR_EACH_NEIGHBOUR(j)
	{
		vec3 p_j = particles[j].position;
		vec3 v_ij = ((p_j - particles[j].oldposition) / timestep) - velocity;
		vec3 p_ij = particle.position - p_j;
		v += v_ij * Wpoly6 (length (p_ij));
		vorticity += cross (v_ij, gradWspiky (p_ij));
	}
	END_FOR_EACH_NEIGHBOUR(j)
	velocity += xsph_viscosity_c * v;
	
	particles[particleid].vorticity = length (vorticity);
	
	barrier ();
	memoryBarrierBuffer ();
	
	vec3 gradVorticity = vec3 (0, 0, 0);
	FOR_EACH_NEIGHBOUR(j)
	{
		vec3 p_ij = particle.position - particles[j].position;
		gradVorticity += particles[j].vorticity * gradWspiky (p_ij);
	}
	END_FOR_EACH_NEIGHBOUR(j)
	
	float l = length (gradVorticity);
	if (l > 0)
		gradVorticity /= l;
	vec3 N = gradVorticity;
	
	// vorticity force
	velocity +=  timestep * vorticity_epsilon * cross (N, vorticity);
	
	barrier ();
	
	// fake old position to update velocity
	particles[particleid].oldposition = particle.position - velocity * timestep;
}
