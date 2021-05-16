#include "VulkanGraphicsResourceCommandBufferManager.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"
#include "VulkanGraphicsResourceSwapchain.h"

VkCommandPool VulkanGraphicsResourceCommandBufferManager::s_CommandGraphicsPool;
std::vector<VkCommandBuffer> VulkanGraphicsResourceCommandBufferManager::s_PrimaryBufferArray;
std::vector<VkCommandBuffer> VulkanGraphicsResourceCommandBufferManager::s_AdditionalBufferArray;

void VulkanGraphicsResourceCommandBufferManager::AllocatePrimaryBufferArray()
{
	int count = VulkanGraphicsResourceSwapchain::GetImageViewCount();
	s_PrimaryBufferArray.resize(count);

	auto allocateInfo = VkCommandBufferAllocateInfo();
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.pNext = NULL;
	allocateInfo.commandPool = s_CommandGraphicsPool;
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
	vkFreeCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_CommandGraphicsPool, s_PrimaryBufferArray.size(), s_PrimaryBufferArray.data());
}

const VkCommandBuffer& VulkanGraphicsResourceCommandBufferManager::GetPrimaryCommandBuffer(int index)
{
	return s_PrimaryBufferArray[index];
}

const VkCommandBuffer& VulkanGraphicsResourceCommandBufferManager::AllocateAdditionalCommandBuffer()
{
	size_t offset = s_AdditionalBufferArray.size();
	s_AdditionalBufferArray.emplace_back();

	auto allocateInfo = VkCommandBufferAllocateInfo();
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.pNext = NULL;
	allocateInfo.commandPool = s_CommandGraphicsPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	auto result = vkAllocateCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), &allocateInfo, s_AdditionalBufferArray.data() + offset);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create the primary command buffer with error code %d\n", result);

		throw;
	}

	return s_AdditionalBufferArray[offset];
}

const std::vector<VkCommandBuffer>& VulkanGraphicsResourceCommandBufferManager::GetAllAdditionalCommandBuffers()
{
	return s_AdditionalBufferArray;
}

void VulkanGraphicsResourceCommandBufferManager::ClearAdditionalCommandBuffer()
{
	vkFreeCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_CommandGraphicsPool, s_AdditionalBufferArray.size(), s_AdditionalBufferArray.data());
	s_AdditionalBufferArray.clear();
}

bool VulkanGraphicsResourceCommandBufferManager::CreateInternal()
{
	auto createInfo = VkCommandPoolCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.queueFamilyIndex = VulkanGraphicsResourceDevice::GetGraphicsQueueFamilyIndex();

	auto result = vkCreateCommandPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &s_CommandGraphicsPool);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a command pool with error code %d\n", result);

		throw;
	}

	return true;
}

bool VulkanGraphicsResourceCommandBufferManager::DestroyInternal()
{
	ClearAdditionalCommandBuffer();
	FreePrimaryBufferArray();
	vkDestroyCommandPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_CommandGraphicsPool, NULL);

	return true;
}