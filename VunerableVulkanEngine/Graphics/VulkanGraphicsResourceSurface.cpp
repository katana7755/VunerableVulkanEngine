#include "VulkanGraphicsResourceSurface.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceInstance.h"

#ifdef _WIN32
#include "vulkan/vulkan_win32.h" // TODO: need to consider other platfroms such as Android, Linux etc... in the future
#endif

VkSurfaceKHR VulkanGraphicsResourceSurface::s_Surface;

#ifdef _WIN32
HINSTANCE VulkanGraphicsResourceSurface::s_HInstance;
HWND VulkanGraphicsResourceSurface::s_HWnd;
#endif

bool VulkanGraphicsResourceSurface::CreateInternal()
{
	VkResult result;

#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.hinstance = s_HInstance;
	createInfo.hwnd = s_HWnd;
	result = vkCreateWin32SurfaceKHR(VulkanGraphicsResourceInstance::GetInstance(), &createInfo, NULL, &s_Surface);
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
	vkDestroySurfaceKHR(VulkanGraphicsResourceInstance::GetInstance(), s_Surface, NULL);
	return true;
}
