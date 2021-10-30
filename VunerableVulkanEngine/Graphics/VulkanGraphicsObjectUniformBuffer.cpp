#include "VulkanGraphicsObjectUniformBuffer.h"
#include "../DebugUtility.h"
#include "../Utils//util_init.hpp"
#include "VulkanGraphicsResourceDevice.h"

bool VulkanGraphicsObjectUniformBuffer::CreateInternal()
{
	// TODO: this is an example code. it will be changed to support all types of uniform buffers
	auto projectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	auto viewMatrix = glm::lookAt(glm::vec3(-5.0f, 3.0f, -10.0f), 
								  glm::vec3(0.0f, 0.0f, 0.0f), 
								  glm::vec3(0.0f, -1.0f, 0.0f));
	auto modelMatrix = glm::mat4(1.0f);
	auto clipMatrix = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, -1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.5f, 0.0f,
								0.0f, 0.0f, 0.5f, 1.0f);
	auto MVPMatrix = clipMatrix * projectionMatrix * viewMatrix * modelMatrix;
	auto createInfo = VkBufferCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	createInfo.size = sizeof(MVPMatrix);
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = NULL;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.flags = 0;
	
	auto result = vkCreateBuffer(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), &createInfo, NULL, &m_Buffer);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create an uniform buffer with error code %d\n", result);
		return false;
	}

	auto requirements = VkMemoryRequirements();
	vkGetBufferMemoryRequirements(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_Buffer, &requirements);

	auto allocationInfo = VkMemoryAllocateInfo();
	allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocationInfo.pNext = NULL;

	if (!VulkanGraphicsResourceDevice::GetInstance().GetMemoryTypeIndex(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocationInfo.memoryTypeIndex))
	{
		printf_console("[VulkanGraphics] cannot find memoryTypeIndex matching VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT\n");
		return false;
	}

	allocationInfo.allocationSize = requirements.size;
	result = vkAllocateMemory(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), &allocationInfo, NULL, &m_DeviceMemory);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create an uniform buffer while allocating VkDeviceMemory with error code %d\n", result);
		return false;
	}

	m_IsDeviceMemoryAllocated = true;

	uint8_t* pData;
	result = vkMapMemory(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_DeviceMemory, 0, requirements.size, 0, (void**)&pData);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create an uniform buffer while mapping memory with error code %d\n", result);
		return false;
	}

	m_BufferSize = sizeof(MVPMatrix);
	memcpy(pData, &MVPMatrix, m_BufferSize);
	vkUnmapMemory(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_DeviceMemory);
	result = vkBindBufferMemory(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_Buffer, m_DeviceMemory, 0);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create an uniform buffer while binding memory with error code %d\n", result);
		return false;
	}

	m_IsDeviceMemoryBound = true;
	return true;
}

bool VulkanGraphicsObjectUniformBuffer::DestroyInternal()
{
	if (m_IsDeviceMemoryBound)
	{
		m_IsDeviceMemoryBound = false;
	}

	if (m_IsDeviceMemoryAllocated)
	{
		vkFreeMemory(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_DeviceMemory, NULL);
		m_IsDeviceMemoryAllocated = false;
	}

	vkDestroyBuffer(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_Buffer, NULL);
	return true;
}