#include "VulkanGraphicsResourceCommandBufferManager.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"
#include "VulkanGraphicsResourceSwapchain.h"

VkCommandPool VulkanGraphicsResourceCommandBufferManager::s_CommandPool;
std::vector<VkCommandBuffer> VulkanGraphicsResourceCommandBufferManager::s_PrimaryBufferArray;
std::vector<VkCommandBuffer> VulkanGraphicsResourceCommandBufferManager::s_SecondaryBufferArray;

void VulkanGraphicsResourceCommandBufferManager::AllocatePrimaryBufferArray()
{
	int count = VulkanGraphicsResourceSwapchain::GetImageViewCount();
	s_PrimaryBufferArray.resize(count);

	auto allocateInfo = VkCommandBufferAllocateInfo();
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.pNext = NULL;
	allocateInfo.commandPool = s_CommandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = count;

	auto result = vkAllocateCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), &allocateInfo, s_PrimaryBufferArray.data());

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create the primary command buffer with error code %d\n", result);

		throw;
	}
}

void VulkanGraphicsResourceCommandBufferManager::FreePrimaryBufferArray()
{
	vkFreeCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_CommandPool, s_PrimaryBufferArray.size(), s_PrimaryBufferArray.data());
}

const VkCommandBuffer& VulkanGraphicsResourceCommandBufferManager::GetPrimaryCommandBuffer(int index)
{
	return s_PrimaryBufferArray[index];
}

bool VulkanGraphicsResourceCommandBufferManager::CreateInternal()
{
	auto createInfo = VkCommandPoolCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.queueFamilyIndex = VulkanGraphicsResourceDevice::GetGraphicsQueueFamilyIndex();

	auto result = vkCreateCommandPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &s_CommandPool);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a command pool with error code %d\n", result);

		throw;
	}

	return true;
}

bool VulkanGraphicsResourceCommandBufferManager::DestroyInternal()
{
	FreePrimaryBufferArray();
	vkDestroyCommandPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_CommandPool, NULL);

	return true;
}