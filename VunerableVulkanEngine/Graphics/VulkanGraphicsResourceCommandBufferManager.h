#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

class VulkanGraphicsResourceCommandBufferManager : public VulkanGraphicsResourceBase
{
public:
	static void AllocatePrimaryBufferArray();
	static void FreePrimaryBufferArray();
	static const VkCommandBuffer& GetPrimaryCommandBuffer(int index);
	static const VkCommandBuffer& AllocateAdditionalCommandBuffer();
	static const std::vector<VkCommandBuffer>& GetAllAdditionalCommandBuffers();
	static void ClearAdditionalCommandBuffer();

private:
	static VkCommandPool s_CommandGraphicsPool;
	static std::vector<VkCommandBuffer> s_PrimaryBufferArray;
	static std::vector<VkCommandBuffer> s_AdditionalBufferArray;

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
};