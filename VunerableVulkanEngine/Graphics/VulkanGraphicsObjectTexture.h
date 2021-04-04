#pragma once
#include "VulkanGraphicsObjectBase.h"

class VulkanGraphicsObjectTexture : public VulkanGraphicsObjectBase
{
public:
	const VkImageView& GetImageView()
	{
		return m_ImageView;
	}

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
	VkImage m_Image;
	bool m_IsDeviceMemoryAllocated;
	VkDeviceMemory m_DeviceMemory;
	bool m_IsDeviceMemoryBound;
	bool m_IsImageViewCreated;
	VkImageView m_ImageView;
};

