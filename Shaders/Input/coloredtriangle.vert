#version 450

vec2 positions[3] = vec2[] (
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

vec3 worldNormals[3] = vec3[] (
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 0.0, 1.0)
);

layout(push_constant) uniform PushConstantsType
{
	mat4x4 MVPMatrix;
	mat4x3 ModelMatrix;
	vec3 MainLightDirection;
} PushConstants;

layout(location = 0) out vec3 fragColor;

void main()
{
	gl_Position = PushConstants.MVPMatrix * vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = dot(worldNormals[gl_VertexIndex], normalize(PushConstants.MainLightDirection)) * vec3(1.0, 1.0, 1.0);
}