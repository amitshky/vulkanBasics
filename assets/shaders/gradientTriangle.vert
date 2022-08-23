#version 450

layout (location = 0) out vec3 fragColor;

// this is normally done in a vertex buffer, 
// but creating one in Vulkan isn't easy
// so this is just for now
vec2 positions[3] = vec2[](
	vec2( 0.0, -0.5),
	vec2( 0.5,  0.5),
	vec2(-0.5,  0.5)
);

// specifying a distinct color for each vertex
// pass this to the fragment shader (so it interpolates these values)
vec3 colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

void main()
{
	// we are directly outputting clip coordinates, w component is set to 1

	// gl_VertexIndex contains index of the current vertex
	// which is usually an index to the vertex buffer,
	// but here it indexes the hardcoded array
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = colors[gl_VertexIndex];
}