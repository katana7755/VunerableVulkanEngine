#pragma once
#include "../Utils/glm/glm.hpp"

struct VertexData
{
	glm::vec3 m_Position;
	glm::vec2 m_UV;
	glm::vec3 m_Normal;
	glm::vec3 m_Color;
};

#pragma once
#include "VulkanGraphicsObjectBase.h"

class VulkanGraphicsObjectMesh : public VulkanGraphicsObjectBase
{
public:
	void PrepareDataFromFBX(const char* strFbxPath);
	const VkBuffer& GetVertexBuffer() { return m_GPUVertexBuffer; };
	const VkBuffer& GetIndexBuffer() { return m_GPUIndexBuffer; };
	uint32_t GetIndexCount() { return m_IndexCount; };

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
	void CreateGPUResource(VkBuffer& gpuBuffer, VkDeviceMemory& gpuMemory, void* dataPtr, VkDeviceSize memorySize, VkBufferUsageFlags bufferUsage, VkFlags memoryFlags);
	void DestroyGPUResource(VkBuffer& gpuBuffer, VkDeviceMemory& gpuMemory);

private:
	// This is for a vertex buffer, so we need to create one more buffer for index buffer...
	VkBuffer m_GPUVertexBuffer;
	VkDeviceMemory m_GPUVertexMemory;
	VkBuffer m_GPUIndexBuffer;
	VkDeviceMemory m_GPUIndexMemory;
	uint32_t m_IndexCount;
};

