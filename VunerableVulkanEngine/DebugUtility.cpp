#pragma once
#include "DebugUtility.h"
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef _WIN32
char g_debug_str[1024];
#endif

#ifdef _WIN32
void printf_console(const char* format, ...)
{
	va_list vl;
	va_start(vl, format);
	vsprintf(g_debug_str, format, vl);
	OutputDebugString(g_debug_str);
	va_end(vl);
}
#endif