#pragma once
#include "../Utils/glm/glm.hpp"

struct VertexData
{
	glm::vec3 m_Position;
	glm::vec2 m_UV;
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
	VkBuffer m_Buffer;
	VkDeviceMemory m_DeviceMemory;
};

