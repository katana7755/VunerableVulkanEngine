#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

class VulkanGraphicsResourceCommandBufferManager : public VulkanGraphicsResourceBase
{
public:
	static const VkCommandPool& GetCommandPool()
	{
		return s_CommandGraphicsPool;
	}

public:
	static void AllocatePrimaryBufferArray();
	static void FreePrimaryBufferArray();
	static const VkCommandBuffer& GetPrimaryCommandBuffer(int index);
	static const VkCommandBuffer& AllocateAdditionalCommandBuffer();
	static void FreeAdditionalCommandBuffer(const VkCommandBuffer& commandBuffer);
	static const std::vector<VkCommandBuffer>& GetAllAdditionalCommandBuffers();
	static void ClearAdditionalCommandBuffers();

private:
	static VkCommandPool s_CommandGraphicsPool;
	static std::vector<VkCommandBuffer> s_PrimaryBufferArray;
	static std::vector<VkCommandBuffer> s_AdditionalBufferArray;
	

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
};