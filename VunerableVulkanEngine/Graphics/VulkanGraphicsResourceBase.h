#pragma once
#include "vulkan/vulkan.h"

class VulkanGraphicsResourceBase
{
public:
	void Create();
	void Destroy();

protected:
	virtual bool CreateInternal() = 0;
	virtual bool DestroyInternal() = 0;

private:
	bool m_IsCreated;
};

