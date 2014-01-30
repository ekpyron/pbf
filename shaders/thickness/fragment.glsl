#version 430 core

// input from vertex shader
in vec2 fTexcoord;

out float thickness;

void main (void)
{
	float r = dot (fTexcoord, fTexcoord);
	if (r > 1)
		discard;	
	thickness = 0.01;
}
