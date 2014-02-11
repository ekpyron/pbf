#version 430 core

// input from vertex shader
in vec2 fTexcoord;
in vec3 fPosition;
flat in uint id;

layout (location = 0) out float thickness;
layout (location = 1) out vec3 noise;

layout (binding = 0) uniform sampler2D depthtex;

uniform bool usenoise;

// projection and view matrix
layout (binding = 0, std140) uniform TransformationBlock {
	mat4 viewmat;
	mat4 projmat;
};

//
// Description : Array and textureless GLSL 2D/3D/4D simplex 
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//
vec4 permute(vec4 x) {
     return mod (((x*34.0)+1.0)*x, 289.0);
}

float snoise(vec3 v)
  { 
// First corner
  vec3 i  = floor(v + dot(v, vec3 (1.0/3.0, 1.0/3.0, 1.0/3.0)) );
  vec3 x0 =   v - i + dot(i, vec3 (1.0/6.0, 1.0/6.0, 1.0/6.0)) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  vec3 x1 = x0 - i1 + 1.0 / 6.0;
  vec3 x2 = x0 - i2 + 1.0 / 3.0;
  vec3 x3 = x0 - 0.5;

// Permutations
  i = mod (i, 289.0); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)

  vec4 j = mod (p, 49);

  vec4 x_ = floor(j / 7);
  vec4 y_ = mod (j, 7);

  vec4 x = x_ * 2.0 / 7.0 - 13.0 / 14.0;
  vec4 y = y_ * 2.0 / 7.0 - 13.0 / 14.0;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  vec4 s0 = vec4(lessThan(b0,vec4 (0.0, 0.0, 0.0, 0.0)))*2.0 - 1.0;
  vec4 s1 = vec4(lessThan(b1,vec4 (0.0, 0.0, 0.0, 0.0)))*2.0 - 1.0;
  
  vec4 a0 = (b0 + s0).xzyw;
  vec4 a1 = (b1 + s1).xzyw;

  vec3 p0 = normalize(vec3(a0.xy,h.x));
  vec3 p1 = normalize(vec3(a0.zw,h.y));
  vec3 p2 = normalize(vec3(a1.xy,h.z));
  vec3 p3 = normalize(vec3(a1.zw,h.w));

// Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                dot(p2,x2), dot(p3,x3) ) );
  }

// linearize a depth value in order to determine a meaningful difference
float linearizeDepth (in float d)
{
	const float f = 1000.0f;
	const float n = 1.0f;
	return (2 * n) / (f + n - d * (f - n));
}

void main (void)
{
	float r = dot (fTexcoord, fTexcoord);
	if (r > 1)
		discard;	
	thickness = 1;
	if (usenoise)
	{
		vec3 normal = vec3 (fTexcoord, -sqrt (1 - r));
		vec4 fPos = vec4 (fPosition - 0.1 * normal, 1.0);
		vec4 clipPos = projmat * fPos;
		float depth = linearizeDepth (clipPos.z / clipPos.w);

		float dd = depth - linearizeDepth (texture (depthtex, fTexcoord).x);
		noise = exp (-r - dd * dd)
				* vec3 (snoise (vec3 (fTexcoord + 1, 16.0 * float (id)) * 0.2),
						snoise (vec3 (fTexcoord + 3, 16.0 * float (id)) * 0.2),
				  		snoise (vec3 (fTexcoord + 5, 16.0 * float (id)) * 0.2));
	}
}
