layout (local_size_x = BLOCKSIZE) in;

struct ParticleKey {
	vec3 pos;
	int id;
};

layout (std430, binding = 1) buffer ParticleKeys
{
	ParticleKey particlekeys[];
};

layout (binding = 2) uniform isamplerBuffer neighbourcelltexture;
layout (binding = 3) uniform samplerBuffer lambdatexture;
layout (binding = 4) uniform sampler3D collisiontexture;

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

void main (void)
{
	vec3 position = particlekeys[gl_GlobalInvocationID.x].pos;

	vec3 deltap = vec3 (0, 0, 0);
	
	//float lambda = lambdas[gl_GlobalInvocationID.x];
	float lambda = texelFetch (lambdatexture, int (gl_GlobalInvocationID.x)).x;
			
	FOR_EACH_NEIGHBOUR(j)
	{
		// This might fetch an already updated position,
		// but that doesn't cause any harm.
		vec3 position_j = particlekeys[j].pos;
		
		float scorr = tensile_instability_scale * Wpoly6 (distance (position, position_j));
		scorr *= scorr;
		scorr *= scorr;
		scorr = -tensile_instability_k * scorr;
		
		float lambda_j = texelFetch (lambdatexture, j).x;  
	
		// accumulate position corrections (part of equation 12)
		deltap += (lambda + lambda_j + scorr) * gradWspiky (position - position_j);
	}
	END_FOR_EACH_NEIGHBOUR(j)

    /*deltap *= one_over_rho_0;

    float dist = length (deltap);
    int num_taps = int (floor (dist)) +1;
    float step = 1.0f / float (num_taps);

    for (int i = 0; i < num_taps; i++) {
        position += step * deltap;
    	vec4 constraint = texture (collisiontexture, position / GRID_SIZE);
	    //vec4 constraint = texelFetch (collisiontexture, ivec3 (position), 0);
	    float epsilon = dot (position.xyz, constraint.xyz) - constraint.w;
	    if (epsilon < 0) {
    		position -= 2*epsilon * constraint.xyz;
    		break;
    	}
    }*/
    position += one_over_rho_0 * deltap;


/*	// collision detection begin
	vec3 wall = vec3 (1, 1, 1);

	vec3 walldist = position - wall;

	position = wall + walldist * (vec3 (greaterThan (walldist, vec3 (0, 0, 0))) * 1.75 - 0.75);

	walldist = (GRID_SIZE - wall) - position;
	position = (GRID_SIZE - wall) - walldist * (vec3 (greaterThan (walldist, vec3 (0, 0, 0))) * 1.75 - 0.75);*/

	vec3 wall = vec3 (16, 0, 16 );

	position = clamp (position, vec3 (0, 0, 0) + wall, GRID_SIZE - wall);
	/*position = clamp (position, vec3 (-16, 0, -16), vec3 (16, 16, 16));*/
	// collision detection end

	particlekeys[gl_GlobalInvocationID.x].pos = position;
}
