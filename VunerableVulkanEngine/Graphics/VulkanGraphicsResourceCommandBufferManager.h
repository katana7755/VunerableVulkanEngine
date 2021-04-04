#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

class VulkanGraphicsResourceCommandBufferManager : public VulkanGraphicsResourceBase
{
public:
	static void AllocatePrimaryBufferArray();
	static void FreePrimaryBufferArray();
	static const VkCommandBuffer& GetPrimaryCommandBuffer(int index);

private:
	static VkCommandPool s_CommandPool;
	static std::vector<VkCommandBuffer> s_PrimaryBufferArray;
	static std::vector<VkCommandBuffer> s_SecondaryBufferArray;

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
};