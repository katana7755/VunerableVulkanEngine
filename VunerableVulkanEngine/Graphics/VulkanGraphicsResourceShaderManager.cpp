#include "VulkanGraphicsResourceShaderManager.h"
#include "VulkanGraphicsResourceDevice.h"
#include "VulnerableUploadBufferManager.h"
#include "../DebugUtility.h"

VulkanGraphicsResourceShaderManager g_Instance;

VulkanGraphicsResourceShaderManager& VulkanGraphicsResourceShaderManager::GetInstance()
{
	return g_Instance;
}

VkShaderModule VulkanGraphicsResourceShaderManager::CreateResourcePhysically(const size_t& uploadBufferIdentifier)
{
    auto inputData = VulnerableUploadBufferManager::GetUploadBuffer(uploadBufferIdentifier);

    auto createInfo = VkShaderModuleCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.codeSize = inputData.m_Size;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(inputData.m_Data);

    auto shaderModule = VkShaderModule();
    auto result = vkCreateShaderModule(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), &createInfo, NULL, &shaderModule);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create a shader module with error code %d\n", result);

        throw;
    }

    return shaderModule;
}

void VulkanGraphicsResourceShaderManager::DestroyResourcePhysicially(const VkShaderModule& shaderModule)
{
    vkDestroyShaderModule(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), shaderModule, NULL);
}