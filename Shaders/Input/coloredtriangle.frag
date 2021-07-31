#version 450

layout(binding = 0) uniform sampler2D samplerDiffuse;

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec2 UV;

layout(location = 0) out vec4 OutColor;

void main()
{
	vec3 color = texture(samplerDiffuse, UV).xyz;
	//color *= FragColor;
	OutColor = vec4(color, 1.0);
}