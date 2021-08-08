#pragma once
#include "VulkanGraphicsResourceBase.h"

#ifdef _WIN32
#include <Windows.h>
#include "../DebugUtility.h"
#endif

class VulkanGraphicsResourceSurface : public VulkanGraphicsResourceBase
{
public:
	static const VkSurfaceKHR& GetSurface()
	{
		return s_Surface;
	}

	static void GetSurfaceSize(uint32_t& outWidth, uint32_t& outHeight)
	{
#ifdef _WIN32
		RECT clientRect;
		
		if (GetClientRect(s_HWnd, &clientRect))
		{
			outWidth = (uint32_t)(clientRect.right - clientRect.left);
			outHeight = (uint32_t)(clientRect.bottom - clientRect.top);
		}
		else
		{
			printf_console("[VulkanGraphics] failed to get surface size\n");

			throw;
		}
#endif
	}

private:
	static VkSurfaceKHR s_Surface;

#ifdef _WIN32
	static HINSTANCE s_HInstance;
	static HWND s_HWnd;
#endif

public:
#ifdef _WIN32
	void Setup(HINSTANCE hInstance, HWND hWnd)
	{
		s_HInstance = hInstance;
		s_HWnd = hWnd;
	}
#endif

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;
};

