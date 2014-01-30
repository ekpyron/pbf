#version 430 core

// enable early z culling
layout (early_fragment_tests) in;

// output color
layout (location = 0) out vec4 color;

// input from vertex shader
in vec2 fTexcoord;

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 viewmat;
	mat4 projmat;
	mat4 invviewmat;
};

layout (binding = 0) uniform sampler2D depthtex;
layout (binding = 1) uniform sampler2D thicknesstex;

// lighting parameters
layout (binding = 1, std140) uniform LightingBuffer
{
	vec3 lightpos;
	vec3 spotdir;
	vec3 eyepos;
	float spotexponent;
	float lightintensity;
};

// obtain position from texcoord and depth
vec3 getpos (in vec3 p)
{
	vec4 pos = inverse (projmat) * vec4 (p * 2 - 1, 1);
	pos /= pos.w;
	pos = inverse (viewmat) * pos;
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

	// view direction
	vec3 viewdir = normalize (eyepos - pos);
	vec3 halfvec = normalize (viewdir + lightdir);
	
	// compute light intensity as the cosine of the angle
	// between light direction and normal direction
	float NdotL = dot (lightdir, normal);
	float intensity = max (NdotL * 0.5 + 0.5, 0);

	// apply distance attenuation and light intensity
	intensity /= lightdist * lightdist;
	intensity *= lightintensity;

	// spot light effect
	float angle = dot (spotdir, -lightdir);
	intensity *= pow (angle, spotexponent);
	
	// specular light
	float k = max (dot (viewdir, reflect (-lightdir, normal)), 0);
	k = pow (k, 8);
	
	// Schlick's approximation for the fresnel term
	float cos_theta = dot (viewdir, lightdir);
	k *= 0.7 + (1 - 0.7) * pow (1 - cos_theta, 5);
	
	// Beer-Lambert law for coloring
	float thickness = texture (thicknesstex, fTexcoord).x;
	vec3 c = vec3 (1 - exp (-0.1 * thickness),
				   1 - exp (-0.5 * thickness),
				   1 - exp (-2.5 * thickness));

	// apply diffuse and specular light to the color value
	color.xyz = clamp (intensity, 0, 1) * c + k * min (c + 0.5, 1);
	// Beer-Lambert law for calculating the alpha value
	color.w =  1 - exp (-2 * thickness);
}
