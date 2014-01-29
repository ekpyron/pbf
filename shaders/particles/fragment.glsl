#version 430 core

// output color
layout (location = 0) out vec4 color;

// input from vertex shader
in vec3 fPosition;
in vec3 fColor;
in vec2 fTexcoord;

// lighting parameters
layout (binding = 1, std140) uniform LightingBuffer
{
	vec3 lightpos;
	vec3 spotdir;
	float spotexponent;
	float lightintensity;
};

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 viewmat;
	mat4 projmat;
	mat4 invviewmat;
};

void main (void)
{
	float r = dot (fTexcoord, fTexcoord);
	if (r > 1)
		discard;
		
	vec3 normal = vec3 (fTexcoord, -sqrt (1 - r));
	
	vec4 fPos = vec4 (fPosition - 0.1 * normal, 1.0);
	vec4 clipPos = projmat * fPos;
	float d = clipPos.z / clipPos.w;
	gl_FragDepth = d;
	
	// lighting calculations

	// obtain light direction and distance
	vec3 lightdir = lightpos - fPos.xyz;
	float lightdist = length (lightdir);
	lightdir /= lightdist;

	// compute light intensity as the cosine of the angle
	// between light direction and normal direction
	float intensity = max (dot (lightdir, normal), 0);

	// apply distance attenuation and light intensity
	intensity /= lightdist * lightdist;
	intensity *= lightintensity;

	// spot light effect
	float angle = dot (spotdir, -lightdir);
	intensity *= pow (angle, spotexponent);

	// ambient light
	intensity += 0.25;

	// fetch texture value and output resulting color
	color = clamp (intensity, 0, 1) * vec4 (fColor, 1);
}
