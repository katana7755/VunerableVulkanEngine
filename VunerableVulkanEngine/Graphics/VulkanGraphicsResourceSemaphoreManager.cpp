#include "VulkanGraphicsResourceSemaphoreManager.h"
#include "VulkanGraphicsResourceDevice.h"

VulkanGraphicsResourceSemaphoreManager g_Instance;

VulkanGraphicsResourceSemaphoreManager& VulkanGraphicsResourceSemaphoreManager::GetInstance()
{
	return g_Instance;
}

VkSemaphore VulkanGraphicsResourceSemaphoreManager::CreateResourcePhysically(const size_t& inputData)
{
	auto outputData = VkSemaphore();
	auto createInfo = VkSemaphoreCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;

	auto result = vkCreateSemaphore(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &outputData);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a semaphore with error code %d\n", result);

		throw;
	}

	return outputData;
}

void VulkanGraphicsResourceSemaphoreManager::DestroyResourcePhysicially(const VkSemaphore& outputData)
{
	vkDestroySemaphore(VulkanGraphicsResourceDevice::GetLogicalDevice(), outputData, NULL);
}