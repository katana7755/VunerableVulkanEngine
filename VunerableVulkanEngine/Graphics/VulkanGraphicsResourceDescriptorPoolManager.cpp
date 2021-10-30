#include "VulkanGraphicsResourceDescriptorPoolManager.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"

VulkanGraphicsResourceDescriptorPoolManager g_Instance;

VulkanGraphicsResourceDescriptorPoolManager& VulkanGraphicsResourceDescriptorPoolManager::GetInstance()
{
	return g_Instance;
}

VkDescriptorPool VulkanGraphicsResourceDescriptorPoolManager::CreateResourcePhysically(const VulkanDescriptorPoolInputData& inputData)
{
    auto createInfo = VkDescriptorPoolCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    createInfo.maxSets = 2;
    createInfo.poolSizeCount = inputData.m_PoolSizeArray.size();
    createInfo.pPoolSizes = inputData.m_PoolSizeArray.data();

    auto descriptorPool = VkDescriptorPool();
    auto result = vkCreateDescriptorPool(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), &createInfo, NULL, &descriptorPool);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create a descriptor pool with error code %d\n", result);

        throw;
    }

    return descriptorPool;
}

void VulkanGraphicsResourceDescriptorPoolManager::DestroyResourcePhysicially(const VkDescriptorPool& descriptorPool)
{
    vkDestroyDescriptorPool(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), descriptorPool, NULL);
}
