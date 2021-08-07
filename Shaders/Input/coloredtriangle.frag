#version 450

layout(binding = 0) uniform sampler2D samplerDiffuseHead;
layout(binding = 1) uniform sampler2D samplerDiffuseBody;

layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 Normal;
layout(location = 2) in float Material;

layout(location = 0) out vec4 OutColor;

void main()
{
	vec3 color = (abs(Material - 0.0) < 0.001) ? texture(samplerDiffuseBody, UV).xyz : vec3(0.0, 0.0, 0.0);
	color += (abs(Material - 1.0) < 0.001) ? texture(samplerDiffuseHead, UV).xyz : vec3(0.0, 0.0, 0.0);
	OutColor = vec4(color, 1.0);
}