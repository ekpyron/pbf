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

layout (std430, binding = 1) buffer LambdaBuffer
{
	float lambdas[];
};

layout (std430, binding = 2) writeonly buffer AuxBuffer
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

	float sum_k_grad_Ci = 0;
	float rho = 0;

	if (particle.highlighted == 1)
		auxdata[particleid] = vec4 (1, 0, 0, 1);
		
	vec3 grad_pi_Ci = vec3 (0, 0, 0);
	
	FOR_EACH_NEIGHBOUR(j)
	{
		vec3 position_j = particles[j].position;
		
		// highlight neighbours of the highlighted particle
		if (particle.highlighted == 1)
		{
			auxdata[j] = vec4 (0, 1, 0, 1);
		}
	
		// compute rho_i (equation 2)
		float len = distance (particle.position, position_j);
		float tmp = Wpoly6 (len);
		rho += tmp;
	
		// sum gradients of Ci (equation 8 and parts of equation 9)
		// use j as k so that we can stay in the same loop
		vec3 grad_pk_Ci = vec3 (0, 0, 0);
		grad_pk_Ci = gradWspiky (particle.position - position_j);
		grad_pk_Ci /= rho_0;
		sum_k_grad_Ci += dot (grad_pk_Ci, grad_pk_Ci);
		
		// now use j as j again and accumulate grad_pi_Ci for the case k=i
		// from equation 8
		grad_pi_Ci += grad_pk_Ci; // = gradWspiky (particle.position - particles[j].position); 
	}
	END_FOR_EACH_NEIGHBOUR(j)
	// add grad_pi_Ci to the sum
	sum_k_grad_Ci += dot (grad_pi_Ci, grad_pi_Ci);
	
	// compute lambda_i (equations 1 and 9)
	float C_i = rho / rho_0 - 1;
	float lambda = -C_i / (sum_k_grad_Ci + epsilon);
	lambdas[particleid] = lambda;
	
	barrier ();
	memoryBarrierBuffer ();
	
	vec3 deltap = vec3 (0, 0, 0);
			
	FOR_EACH_NEIGHBOUR(j)
	{
		vec3 position_j = particles[j].position;
		
		float scorr = (Wpoly6 (distance (particle.position, position_j)) / Wpoly6 (tensile_instability_h));
		scorr *= scorr;
		scorr *= scorr;
		scorr = -tensile_instability_k * scorr;  
	
		// accumulate position corrections (part of equation 12)
		deltap += (lambda + lambdas[j] + scorr) * gradWspiky (particle.position - position_j);
	}
	END_FOR_EACH_NEIGHBOUR(j)

	particle.position += deltap / rho_0;

	// collision detection begin
	vec3 wall = vec3 (16, 0, 16);
	particle.position = clamp (particle.position, vec3 (0, 0, 0) + wall, GRID_SIZE - wall);
	// collision detection end
	
	particles[particleid].position = particle.position;
}
