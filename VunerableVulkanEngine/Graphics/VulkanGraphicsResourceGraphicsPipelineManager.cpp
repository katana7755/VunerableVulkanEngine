#include "VulkanGraphicsResourceGraphicsPipelineManager.h"
#include "VulkanGraphicsResourceDevice.h"
#include "VulkanGraphicsResourcePipelineCache.h"

VulkanGraphicsResourceGraphicsPipelineManager g_Instance;

VulkanGraphicsResourceGraphicsPipelineManager& VulkanGraphicsResourceGraphicsPipelineManager::GetInstance()
{
	return g_Instance;
}

VkPipeline VulkanGraphicsResourceGraphicsPipelineManager::CreateResourcePhysically(const VulkanGraphicsPipelineInputData& inputData)
{
	auto createInfo = GenerateCreateInfo(inputData);
	auto newPipeline = VkPipeline();
	auto result = vkCreateGraphicsPipelines(VulkanGraphicsResourceDevice::GetLogicalDevice(), VulkanGraphicsResourcePipelineCache::GetInstance().GetPipelineCache(), 1, &createInfo, NULL, &newPipeline);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create graphics pipelines with error code %d\n", result);

        throw;
    }

    
    return newPipeline;
}

void VulkanGraphicsResourceGraphicsPipelineManager::DestroyResourcePhysicially(const VkPipeline& pipeline)
{
    vkDestroyPipeline(VulkanGraphicsResourceDevice::GetLogicalDevice(), pipeline, NULL);
}

VkGraphicsPipelineCreateInfo VulkanGraphicsResourceGraphicsPipelineManager::GenerateCreateInfo(const VulkanGraphicsPipelineInputData& inputData)
{
    auto createInfo = VkGraphicsPipelineCreateInfo();

    return createInfo;
}