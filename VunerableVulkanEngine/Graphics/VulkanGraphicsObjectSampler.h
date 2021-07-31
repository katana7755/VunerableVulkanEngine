#pragma once
#include "VulkanGraphicsObjectBase.h"

class VulkanGraphicsObjectSampler : public VulkanGraphicsObjectBase
{
public:
	const VkSampler& GetSampler()
	{
		return m_Sampler;
	}
	
protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
	VkSampler m_Sampler;
};