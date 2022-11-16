#version 450

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec3 inColor;

layout (location = 0) out vec3 fragColor;

// uniforms
layout (binding = 0) uniform UniformBufferObjects
{
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;


void main()
{
	// we are directly outputting clip coordinates, w component is set to 1

	// gl_VertexIndex contains index of the current vertex
	// which is usually an index to the vertex buffer,
	// but here it indexes the hardcoded array
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
	fragColor = inColor;
}