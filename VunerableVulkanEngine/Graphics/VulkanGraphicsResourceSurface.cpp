#include "VulkanGraphicsResourceSurface.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceInstance.h"

#ifdef _WIN32
#include "vulkan/vulkan_win32.h" // TODO: need to consider other platfroms such as Android, Linux etc... in the future
#endif

VulkanGraphicsResourceSurface g_Instance;

VulkanGraphicsResourceSurface& VulkanGraphicsResourceSurface::GetInstance()
{
	return g_Instance;
}

bool VulkanGraphicsResourceSurface::CreateInternal()
{
	VkResult result;

#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.hinstance = m_HInstance;
	createInfo.hwnd = m_HWnd;
	result = vkCreateWin32SurfaceKHR(VulkanGraphicsResourceInstance::GetInstance().GetVkInstance(), &createInfo, NULL, &m_Surface);
#endif

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a windows surface with error code %d\n", result);

		throw;
	}

	return result == VK_SUCCESS;
}

bool VulkanGraphicsResourceSurface::DestroyInternal()
{
	vkDestroySurfaceKHR(VulkanGraphicsResourceInstance::GetInstance().GetVkInstance(), m_Surface, NULL);
	return true;
}
