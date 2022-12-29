#version 450

// specify the index of the framebuffer
// the color is written to the `outColor` that is linked to the first framebuffer at index 0
layout (location = 0) out vec4 outColor;
layout (location = 0) in  vec3 fragColor; // from the vertex shader
layout (location = 1) in  vec2 fragTexCoord;

layout (binding = 1) uniform sampler2D texSampler;

void main()
{
	outColor = texture(texSampler, fragTexCoord);
}
