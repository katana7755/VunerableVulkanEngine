#pragma once

#include "VulkanGraphicsResourceBase.h"

class VulkanGraphicsResourcePipelineCache : VulkanGraphicsResourceBase
{
public:
	static VulkanGraphicsResourcePipelineCache& GetInstance();

public:
	const VkPipelineCache& GetPipelineCache()
	{
		return m_PipelineCache;
	}

protected:
	virtual bool CreateInternal();
	virtual bool DestroyInternal();

private:
	VkPipelineCache m_PipelineCache;
};