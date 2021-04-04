#pragma once
#include "VulkanGraphicsObjectBase.h"

class VulkanGraphicsObjectUniformBuffer : public VulkanGraphicsObjectBase
{
public:
	VkBuffer GetBuffer()
	{
		return m_Buffer;
	}

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
	VkBuffer m_Buffer;
	int m_BufferSize;
	bool m_IsDeviceMemoryAllocated;
	VkDeviceMemory m_DeviceMemory;
	bool m_IsDeviceMemoryBound;
};

