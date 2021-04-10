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
	VkDeviceMemory m_DeviceMemory;
	VkImageView m_ImageView;
};

