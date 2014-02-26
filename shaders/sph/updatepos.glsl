// include.glsl is included here
#line 3

layout (local_size_x = 256) in;

layout (std430, binding = 0) buffer ParticleKeys
{
	ParticleKey particlekeys[];
};

layout (binding = 2) uniform isamplerBuffer neighbourcelltexture;
layout (binding = 3) uniform samplerBuffer lambdatexture;

float Wpoly6 (float r)
{
	if (r > h)
		return 0;
	float tmp = h * h - r * r;
	return 1.56668147106 * tmp * tmp * tmp / (h*h*h*h*h*h*h*h*h);
}

float Wspiky (float r)
{
	if (r > h)
		return 0;
	float tmp = h - r;
	return 4.774648292756860 * tmp * tmp * tmp / (h*h*h*h*h*h);
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
	vec3 position = particlekeys[gl_GlobalInvocationID.x].position;

	vec3 deltap = vec3 (0, 0, 0);
	
	//float lambda = lambdas[gl_GlobalInvocationID.x];
	float lambda = texelFetch (lambdatexture, int (gl_GlobalInvocationID.x)).x;
			
	FOR_EACH_NEIGHBOUR(j)
	{
		// This might fetch an already updated position,
		// but that doesn't cause any harm.
		vec3 position_j = particlekeys[j].position;
		
		float scorr = (Wpoly6 (distance (position, position_j)) / Wpoly6 (tensile_instability_h));
		scorr *= scorr;
		scorr *= scorr;
		scorr = -tensile_instability_k * scorr;
		
		float lambda_j = texelFetch (lambdatexture, j).x;  
	
		// accumulate position corrections (part of equation 12)
		deltap += (lambda + lambda_j + scorr) * gradWspiky (position - position_j);
	}
	END_FOR_EACH_NEIGHBOUR(j)

	position += deltap / rho_0;

	// collision detection begin
	vec3 wall = vec3 (16, 0, 16);
	position = clamp (position, vec3 (0, 0, 0) + wall, GRID_SIZE - wall);
	// collision detection end
	
	particlekeys[gl_GlobalInvocationID.x].position = position;
}
