#include "VulkanGraphicsObjectSampler.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"

bool VulkanGraphicsObjectSampler::CreateInternal()
{
	// TODO: Need to support more degree of freedom, meaning we will be able to create any type of samplers
	auto createInfo = VkSamplerCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0; // TODO: Need to figure out when we need this...
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	createInfo.mipLodBias = 0.0f;
	createInfo.anisotropyEnable = VK_FALSE;
	createInfo.maxAnisotropy = 0.0f;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;
	createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;

	auto result = vkCreateSampler(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &m_Sampler);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a sampler with error code %d\n", result);
		throw;
	}

	return true;
}

bool VulkanGraphicsObjectSampler::DestroyInternal()
{
	vkDestroySampler(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_Sampler, NULL);

	return true;
}