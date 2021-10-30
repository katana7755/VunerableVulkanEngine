#include "VulkanGraphicsResourceFrameBufferManager.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"
#include "VulkanGraphicsResourceRenderPassManager.h"

VulkanGraphicsResourceFrameBufferManager g_Instance;

VulkanGraphicsResourceFrameBufferManager& VulkanGraphicsResourceFrameBufferManager::GetInstance()
{
	return g_Instance;
}

VkFramebuffer VulkanGraphicsResourceFrameBufferManager::CreateResourcePhysically(const VulkanFrameBufferInputData& inputData)
{
	auto createInfo = VkFramebufferCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0; // TODO: figure out what is the advantage of the imageless flag...
	createInfo.renderPass = VulkanGraphicsResourceRenderPassManager::GetInstance().GetResource(inputData.m_RenderPassIdentifier);
	createInfo.attachmentCount = inputData.m_AttachmentArray.size();
	createInfo.pAttachments = inputData.m_AttachmentArray.data();
	createInfo.width = inputData.m_Width;
	createInfo.height = inputData.m_Height;
	createInfo.layers = inputData.m_Layers;

	auto frameBuffer = VkFramebuffer();
	auto result = vkCreateFramebuffer(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), &createInfo, NULL, &frameBuffer);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a framebuffer with error code %d\n", result);

		throw;
	}

	return frameBuffer;
}

void VulkanGraphicsResourceFrameBufferManager::DestroyResourcePhysicially(const VkFramebuffer& frameBuffer)
{
	vkDestroyFramebuffer(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), frameBuffer, NULL);
}
