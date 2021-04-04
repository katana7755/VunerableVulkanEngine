#pragma once
#include "VulkanGraphicsResourceBase.h"

class VulkanGraphicsResourceInstance : public VulkanGraphicsResourceBase
{
public:
	static VkInstance GetInstance()
	{
		return s_Instance;
	}

private:
	static VkInstance s_Instance;

#if _DEBUG
	static VkDebugUtilsMessengerEXT s_DebugCallbackMessenger;
#endif

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

#if _DEBUG
	VkResult CreateDebugUtilsMessenger();
	void DestroyDebugUtilsMessenger();
#endif
};

