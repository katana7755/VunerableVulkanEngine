#include "VulkanGraphicsResourceRenderPassManager.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"

std::vector<VkRenderPass> VulkanGraphicsResourceRenderPassManager::s_RenderPassArray;
std::vector<VkFramebuffer> VulkanGraphicsResourceRenderPassManager::s_FramebufferArray;

int VulkanGraphicsResourceRenderPassManager::CreateRenderPass(const std::vector<VkAttachmentDescription>& attachmentDescArray, const std::vector<VkSubpassDescription>& subpassDescArray, const std::vector<VkSubpassDependency>& subpassDepArray)
{
	auto createInfo = VkRenderPassCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.attachmentCount = attachmentDescArray.size();
	createInfo.pAttachments = attachmentDescArray.data();
	createInfo.subpassCount = subpassDescArray.size();
	createInfo.pSubpasses = subpassDescArray.data();
	createInfo.dependencyCount = subpassDepArray.size();
	createInfo.pDependencies = subpassDepArray.data();

	VkRenderPass renderPass;
	auto result = vkCreateRenderPass(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &renderPass);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a render pass with error code %d\n", result);

		throw;
	}

	s_RenderPassArray.push_back(renderPass);

	return s_RenderPassArray.size() - 1;
}

const VkRenderPass& VulkanGraphicsResourceRenderPassManager::GetRenderPass(int index)
{
	return s_RenderPassArray[index];
}

void VulkanGraphicsResourceRenderPassManager::DestroyRenderPass(int index)
{
	vkDestroyRenderPass(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_RenderPassArray[index], NULL);
	s_RenderPassArray.erase(s_RenderPassArray.begin() + index);
}

int VulkanGraphicsResourceRenderPassManager::CreateFramebuffer(int renderPassIndex, std::vector<VkImageView> attachmentArray, uint32_t width, uint32_t height, uint32_t layers)
{
	auto createInfo = VkFramebufferCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0; // TODO: figure out what is the advantage of the imageless flag...
	createInfo.renderPass = s_RenderPassArray[renderPassIndex];
	createInfo.attachmentCount = attachmentArray.size();
	createInfo.pAttachments = attachmentArray.data();
	createInfo.width = width;
	createInfo.height = height;
	createInfo.layers = layers;

	auto framebuffer = VkFramebuffer();
	auto result = vkCreateFramebuffer(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &framebuffer);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a framebuffer with error code %d\n", result);

		throw;
	}

	s_FramebufferArray.push_back(framebuffer);

	return s_FramebufferArray.size() - 1;
}

const VkFramebuffer& VulkanGraphicsResourceRenderPassManager::GetFramebuffer(int index)
{
	return s_FramebufferArray[index];
}

void VulkanGraphicsResourceRenderPassManager::DestroyFramebuffer(int index)
{
	vkDestroyFramebuffer(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_FramebufferArray[index], NULL);
	s_FramebufferArray.erase(s_FramebufferArray.begin() + index);
}

bool VulkanGraphicsResourceRenderPassManager::CreateInternal()
{
	return true;
}

bool VulkanGraphicsResourceRenderPassManager::DestroyInternal()
{
	for (auto framebuffer : s_FramebufferArray)
	{
		vkDestroyFramebuffer(VulkanGraphicsResourceDevice::GetLogicalDevice(), framebuffer, NULL);
	}

	s_FramebufferArray.clear();

	for (auto renderPass : s_RenderPassArray)
	{
		vkDestroyRenderPass(VulkanGraphicsResourceDevice::GetLogicalDevice(), renderPass, NULL);
	}

	s_RenderPassArray.clear();

	return true;
}