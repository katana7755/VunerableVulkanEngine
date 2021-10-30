#include "VulkanGraphicsResourceRenderPassManager.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"

VulkanGraphicsResourceRenderPassManager g_Instance;

VulkanGraphicsResourceRenderPassManager& VulkanGraphicsResourceRenderPassManager::GetInstance()
{
	return g_Instance;
}

VkRenderPass VulkanGraphicsResourceRenderPassManager::CreateResourcePhysically(const VulkanRenderPassInputData& inputData)
{
	auto createInfo = VkRenderPassCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.attachmentCount = inputData.m_AttachmentDescriptionArray.size();
	createInfo.pAttachments = inputData.m_AttachmentDescriptionArray.data();
	createInfo.subpassCount = inputData.m_SubpassDescriptionArray.size();
	createInfo.pSubpasses = inputData.m_SubpassDescriptionArray.data();
	createInfo.dependencyCount = inputData.m_SubpassDependencyArray.size();
	createInfo.pDependencies = inputData.m_SubpassDependencyArray.data();

	VkRenderPass renderPass;
	auto result = vkCreateRenderPass(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), &createInfo, NULL, &renderPass);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a render pass with error code %d\n", result);

		throw;
	}

	return renderPass;
}

void VulkanGraphicsResourceRenderPassManager::DestroyResourcePhysicially(const VkRenderPass& renderPass)
{
	vkDestroyRenderPass(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), renderPass, NULL);
}
