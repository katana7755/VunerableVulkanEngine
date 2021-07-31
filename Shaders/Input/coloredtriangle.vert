#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 VertexColor;

layout(push_constant) uniform PushConstantsType
{
	mat4x4 MVPMatrix;
	vec3 MainLightDirection;
} PushConstants;

layout(location = 0) out vec3 OutNormal;
layout(location = 1) out vec2 OutUV;

void main()
{
	gl_Position = PushConstants.MVPMatrix * vec4(Position, 1.0);

	//vec3 worldNormal = Normal; // ***** TODO: we need to convert this into world space normal, and for that TBN matrix is necessary...
	//OutFragColor = dot(worldNormal, normalize(PushConstants.MainLightDirection)) * VertexColor;
	OutNormal = Normal;
	OutUV = UV;
}