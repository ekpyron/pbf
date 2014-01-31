#version 430 core

// input from vertex shader
in vec2 fTexcoord;

void main (void)
{
	float r = dot (fTexcoord, fTexcoord);
	if (r > 0.5)
	{
		discard;
		return;
	}
}
