#version 430 core

// enable early z culling
layout (early_fragment_tests) in;

// output color
layout (location = 0) out vec4 color;

// input from vertex shader
in vec2 fTexcoord;

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 mvpmat;
	mat4 invpmat;
};

layout (binding = 0) uniform sampler2D depthtex;

// lighting parameters
layout (binding = 1, std140) uniform LightingBuffer
{
	vec3 lightpos;
	vec3 spotdir;
	float spotexponent;
	float lightintensity;
};

// obtain position from texcoord and depth
vec3 getpos (vec3 p)
{
	vec4 pos = invpmat * vec4 (2 * p - 1, 1);
	return pos.xyz / pos.w;
}

void main (void)
{
	// fetch depth from texture
	float depth = texture (depthtex, fTexcoord, 0).x;
	if (depth == 1.0)
		discard;
		
	vec3 pos = getpos (vec3 (fTexcoord, depth));

	vec2 tmp;
	tmp = fTexcoord + vec2 (1.0f / 1280.0f, 0);
	vec3 ddx = getpos (vec3 (tmp, texture (depthtex, tmp, 0).x)) - pos;
	tmp = fTexcoord - vec2 (1.0f / 1280.0f, 0);
	vec3 ddx2 = pos - getpos (vec3 (tmp, texture (depthtex, tmp, 0).x));
	if (abs (ddx.z) > abs (ddx2.z)) ddx = ddx2;

	tmp = fTexcoord + vec2 (0, 1.0f / 720.0f);
	vec3 ddy = getpos (vec3 (tmp, texture (depthtex, tmp, 0).x)) - pos;
	tmp = fTexcoord - vec2 (0, 1.0f / 720.0f);
	vec3 ddy2 = pos - getpos (vec3 (tmp, texture (depthtex, tmp, 0).x));
	if (abs (ddy.z) < abs (ddy2.z)) ddy = ddy2;
	
	vec3 normal = normalize (cross (ddx, ddy));
	
	
	// lighting calculations

	// obtain light direction and distance
	vec3 lightdir = lightpos - pos;
	float lightdist = length (lightdir);
	lightdir /= lightdist;

	// compute light intensity as the cosine of the angle
	// between light direction and normal direction
	float intensity = max (dot (lightdir, normal) * 0.5 + 0.5, 0);

	// apply distance attenuation and light intensity
	intensity /= lightdist * lightdist;
	intensity *= lightintensity;

	// spot light effect
	float angle = dot (spotdir, -lightdir);
	intensity *= pow (angle, spotexponent);

	// fetch texture value and output resulting color
	color = clamp (intensity, 0, 1) * vec4 (0.25, 0, 1, 1);
	
	
	
	// output position as color
	//color = vec4 (normal * 0.5 + 0.5, 1); //0.1 * vec4 (pos1, 1);
}
