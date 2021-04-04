#include "VulkanGraphicsObjectBase.h"

void VulkanGraphicsObjectBase::Create()
{
	Destroy();
	
	if (!CreateInternal())
	{
		return;
	}

	m_IsCreated = true;
}

void VulkanGraphicsObjectBase::Destroy()
{
	if (!m_IsCreated)
	{
		return;
	}

	if (!DestroyInternal())
	{
		return;
	}

	m_IsCreated = false;
}