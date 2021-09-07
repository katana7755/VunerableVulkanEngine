#include "VulkanGraphicsResourceShaderManager.h"
#include "VulkanGraphicsResourceDevice.h"
#include "../DebugUtility.h"

VulkanGraphicsResourceShaderManager g_Instance;

VulkanGraphicsResourceShaderManager& VulkanGraphicsResourceShaderManager::GetInstance()
{
	return g_Instance;
}

void VulkanGraphicsResourceShaderManager::CreateResourcePhysically(const size_t identifier, const char* data, const unsigned int size)
{
    auto createInfo = VkShaderModuleCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.codeSize = size;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(data);

    auto shaderModule = VkShaderModule();
    auto result = vkCreateShaderModule(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &shaderModule);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create a shader module with error code %d\n", result);

        throw;
    }

    size_t actualIndex = m_IdentifierArray[identifier];
    m_ResourceArray[actualIndex] = shaderModule;
}

void VulkanGraphicsResourceShaderManager::DestroyResourcePhysicially(const size_t identifier)
{
    size_t actualIndex = m_IdentifierArray[identifier];
    vkDestroyShaderModule(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_ResourceArray[actualIndex], NULL);
}