#version 450

// specify the index of the framebuffer
// the color is written to the `outColor` that is linked to the first framebuffer at index 0
layout (location = 0) out vec4 outColor;
layout (location = 0) in  vec3 fragColor; // from the vertex shader

void main()
{
	outColor = vec4(fragColor, 1.0);
}