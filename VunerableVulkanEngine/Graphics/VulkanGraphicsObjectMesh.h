#pragma once
#include "../Utils/glm/glm.hpp"

struct VertexData
{
	glm::vec3 m_Position;
	glm::vec2 m_UV;
	glm::vec3 m_Normal;
	glm::vec4 m_Color;
};

#pragma once
#include "VulkanGraphicsObjectBase.h"

class VulkanGraphicsObjectMesh : public VulkanGraphicsObjectBase
{
public:
	void PrepareDataFromFBX(const char* strFbxPath);

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
	// This is for a vertex buffer, so we need to create one more buffer for index buffer...
	VkBuffer m_GPUBuffer;
	VkDeviceMemory m_GPUMemory;
};

