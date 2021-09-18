#include "VulkanGraphicsResourceDescriptorSetLayoutManager.h"
#include "VulkanGraphicsResourceDevice.h"
#include "../DebugUtility.h"

VulkanGraphicsResourceDescriptorSetLayoutManager g_Instance;

VulkanGraphicsResourceDescriptorSetLayoutManager& VulkanGraphicsResourceDescriptorSetLayoutManager::GetInstance()
{
	return g_Instance;
}

VkDescriptorSetLayout VulkanGraphicsResourceDescriptorSetLayoutManager::CreateResourcePhysically(const VulkanShaderMetaData& shaderMetaData)
{
    auto bindingInfoArray = std::vector<VkDescriptorSetLayoutBinding>();

    for (int i = 0; i < shaderMetaData.m_InputBindingArray.size(); ++i)
    {
        auto inputBinding = shaderMetaData.m_InputBindingArray[i];

        if (inputBinding == EVulkanShaderBindingResource_NONE)
        {
            continue;
        }

        auto bindingInfo = VkDescriptorSetLayoutBinding();
        bindingInfo.binding = i;

        switch (inputBinding)
        {
        case EVulkanShaderBindingResource::TEXTURE2D:
            bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            break;
        default:
            printf_console("[VulkanGraphics] not a valid binding type %d\n", inputBinding);

            throw;
        }

        bindingInfo.descriptorCount = 1; // TODO: this also seems useful... but still don't understand what this is for exactly...
        bindingInfo.stageFlags = shaderMetaData.GetShaderStageBits();
        bindingInfo.pImmutableSamplers = NULL; // TODO: this also seems useful... but still don't understand what this is for exactly...
        bindingInfoArray.push_back(bindingInfo);
    }

    auto createInfo = VkDescriptorSetLayoutCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0; // TODO: this also seems useful... but still don't understand what this is for exactly...
    createInfo.bindingCount = bindingInfoArray.size();
    createInfo.pBindings = bindingInfoArray.data();

    auto newLayout = VkDescriptorSetLayout();
    auto result = vkCreateDescriptorSetLayout(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &newLayout);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create a descriptor set layout with error code %d\n", result);

        throw;
    }

    return newLayout;
}

void VulkanGraphicsResourceDescriptorSetLayoutManager::DestroyResourcePhysicially(const VkDescriptorSetLayout& descriptorSetLayout)
{
    vkDestroyDescriptorSetLayout(VulkanGraphicsResourceDevice::GetLogicalDevice(), descriptorSetLayout, NULL);
}