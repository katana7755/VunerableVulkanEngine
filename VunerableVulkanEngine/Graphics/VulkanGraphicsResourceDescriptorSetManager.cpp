#include "VulkanGraphicsResourceDescriptorSetManager.h"
#include "VulkanGraphicsResourceGraphicsPipelineManager.h"
#include "VulkanGraphicsResourceShaderManager.h"
#include "VulkanGraphicsResourceDescriptorSetLayoutManager.h"
#include "VulkanGraphicsResourceDescriptorPoolManager.h"
#include "VulkanGraphicsResourceDevice.h"
#include "../DebugUtility.h"

VulkanGraphicsResourceDescriptorSetManager g_Instance;

void VulkanDescriptorSetOutputData::ClearBindingInfos()
{
    for (auto& writeSet : m_WriteSetArray)
    {
        if (writeSet.pImageInfo != NULL)
        {
            delete writeSet.pImageInfo;
            writeSet.pImageInfo = NULL;
        }

        if (writeSet.pBufferInfo != NULL)
        {
            delete writeSet.pBufferInfo;
            writeSet.pBufferInfo = NULL;
        }

        if (writeSet.pTexelBufferView != NULL)
        {
            delete writeSet.pTexelBufferView;
            writeSet.pTexelBufferView = NULL;
        }
    }

    m_WriteSetArray.clear();
}

void VulkanDescriptorSetOutputData::BindCombinedSampler(uint32_t binding, const VkImage& image, const VkImageView& imageView, const VkSampler& sampler)
{
    auto& pipelineData = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().GetResource(m_PipelineIdentifier);
    auto& shaderMetaData = VulkanGraphicsResourceShaderManager::GetInstance().GetResourceKey(pipelineData.m_ShaderIdentifiers[m_ShaderType]);

    if (binding >= shaderMetaData.m_InputBindingArray.size())
    {
        printf_console("[VulkanGraphics] binding index %d exceeds binding count %d\n", binding, shaderMetaData.m_InputBindingArray.size());

        throw;
    }

    if (shaderMetaData.m_InputBindingArray[binding] != EVulkanShaderBindingResource::TEXTURE2D)
    {
        printf_console("[VulkanGraphics] binding index %d is not for a texture binding\n");

        throw;
    }

    auto writeDescriptorSet = VkWriteDescriptorSet();
    auto* pImageInfo = new VkDescriptorImageInfo();
    pImageInfo->sampler = sampler;
    pImageInfo->imageView = imageView;
    pImageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    auto writeSet = VkWriteDescriptorSet();
    writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSet.pNext = NULL;
    writeSet.dstSet = m_DescriptorSet;
    writeSet.dstBinding = binding;
    writeSet.dstArrayElement = 0;
    writeSet.descriptorCount = 1;
    writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSet.pImageInfo = pImageInfo;
    writeSet.pBufferInfo = NULL;
    writeSet.pTexelBufferView = NULL;
    m_WriteSetArray.push_back(writeSet);
}

void VulkanDescriptorSetOutputData::FlushBindingInfos()
{
    vkUpdateDescriptorSets(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_WriteSetArray.size(), m_WriteSetArray.data(), 0, NULL);
}

VulkanGraphicsResourceDescriptorSetManager& VulkanGraphicsResourceDescriptorSetManager::GetInstance()
{
	return g_Instance;
}

VulkanDescriptorSetOutputData VulkanGraphicsResourceDescriptorSetManager::CreateResourcePhysically(const VulkanDescriptorSetInputData& inputData)
{
    auto& pipelineData = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().GetResource(inputData.m_PipelineIdentifier);
    auto& descriptorSetLayout = VulkanGraphicsResourceDescriptorSetLayoutManager::GetInstance().GetResource(pipelineData.m_DescriptorSetLayoutIdentifiers[inputData.m_ShaderType]);

    auto allocateInfo = VkDescriptorSetAllocateInfo();
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.pNext = NULL;
    allocateInfo.descriptorPool = VulkanGraphicsResourceDescriptorPoolManager::GetInstance().GetResource(inputData.m_DescriptorPoolIdentifier);
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &descriptorSetLayout;

    auto outputData = VulkanDescriptorSetOutputData();
    outputData.m_DescriptorPoolIdentifier = inputData.m_DescriptorPoolIdentifier;
    outputData.m_PipelineIdentifier = inputData.m_PipelineIdentifier;
    outputData.m_ShaderType = inputData.m_ShaderType;

    auto result = vkAllocateDescriptorSets(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), &allocateInfo, &outputData.m_DescriptorSet);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to allocate a descriptor set with error code %d\n", result);

        throw;
    }

    return outputData;
}

void VulkanGraphicsResourceDescriptorSetManager::DestroyResourcePhysicially(const VulkanDescriptorSetOutputData& outputData)
{
    const_cast<VulkanDescriptorSetOutputData&>(outputData).ClearBindingInfos();

    auto result = vkFreeDescriptorSets(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), VulkanGraphicsResourceDescriptorPoolManager::GetInstance().GetResource(outputData.m_DescriptorPoolIdentifier), 1, &outputData.m_DescriptorSet);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to allocate a descriptor set with error code %d\n", result);

        throw;
    }
}