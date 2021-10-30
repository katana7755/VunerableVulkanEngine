#pragma once
#include "VulkanGraphicsResourceBase.h"

class VulkanGraphicsResourceInstance : public VulkanGraphicsResourceBase
{
public:
	static VulkanGraphicsResourceInstance& GetInstance();

public:
	const VkInstance& GetVkInstance()
	{
		return m_VkInstance;
	}

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

#if _DEBUG
	VkResult CreateDebugUtilsMessenger();
	void DestroyDebugUtilsMessenger();
#endif

private:
	VkInstance m_VkInstance;

#if _DEBUG
	VkDebugUtilsMessengerEXT m_DebugCallbackMessenger;
#endif
};

