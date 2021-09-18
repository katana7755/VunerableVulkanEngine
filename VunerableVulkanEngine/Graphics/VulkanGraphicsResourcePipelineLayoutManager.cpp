#include "VulkanGraphicsResourcePipelineLayoutManager.h"
#include "VulkanGraphicsResourceDevice.h"

VulkanGraphicsResourcePipelineLayoutManager g_Instance;

VulkanGraphicsResourcePipelineLayoutManager& VulkanGraphicsResourcePipelineLayoutManager::GetInstance()
{
	return g_Instance;
}

VkPipelineLayout VulkanGraphicsResourcePipelineLayoutManager::CreateResourcePhysically(const VulkanPipelineLayoutInputData& inputData)
{
    auto createInfo = VkPipelineLayoutCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.setLayoutCount = inputData.m_DescriptorSetLayoutArray.size();
    createInfo.pSetLayouts = inputData.m_DescriptorSetLayoutArray.data();
    createInfo.pushConstantRangeCount = inputData.m_PushConstantRangeArray.size();
    createInfo.pPushConstantRanges = inputData.m_PushConstantRangeArray.data();;

    auto newLayout = VkPipelineLayout();
	auto result = vkCreatePipelineLayout(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &newLayout);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create a pipeline layout with error code %d\n", result);

        throw;
    }

    return newLayout;
}

void VulkanGraphicsResourcePipelineLayoutManager::DestroyResourcePhysicially(const VkPipelineLayout& pipelineLayout)
{
	vkDestroyPipelineLayout(VulkanGraphicsResourceDevice::GetLogicalDevice(), pipelineLayout, NULL);
}