#pragma once
#include "VulkanGraphicsResourceBase.h"

#ifdef _WIN32
#include <Windows.h>
#include "../DebugUtility.h"
#endif

class VulkanGraphicsResourceSurface : public VulkanGraphicsResourceBase
{
public:
	static VulkanGraphicsResourceSurface& GetInstance();

public:
	const VkSurfaceKHR& GetSurface()
	{
		return m_Surface;
	}

	void GetSurfaceSize(uint32_t& outWidth, uint32_t& outHeight)
	{
#ifdef _WIN32
		RECT clientRect;

		if (GetClientRect(m_HWnd, &clientRect))
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

#ifdef _WIN32
	void Setup(HINSTANCE hInstance, HWND hWnd)
	{
		m_HInstance = hInstance;
		m_HWnd = hWnd;
	}
#endif

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
	VkSurfaceKHR m_Surface;

#ifdef _WIN32
	HINSTANCE m_HInstance;
	HWND m_HWnd;
#endif
};

