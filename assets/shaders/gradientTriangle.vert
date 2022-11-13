#version 450

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec3 inColor;

layout (location = 0) out vec3 fragColor;

void main()
{
	// we are directly outputting clip coordinates, w component is set to 1

	// gl_VertexIndex contains index of the current vertex
	// which is usually an index to the vertex buffer,
	// but here it indexes the hardcoded array
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = colors[gl_VertexIndex];
}