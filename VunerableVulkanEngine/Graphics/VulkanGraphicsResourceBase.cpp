#include "VulkanGraphicsResourceBase.h"

void VulkanGraphicsResourceBase::Create()
{
	Destroy();

	if (!CreateInternal())
	{
		throw;
	}

	m_IsCreated = true;
}

void VulkanGraphicsResourceBase::Destroy()
{
	if (!m_IsCreated)
	{
		return;
	}

	m_IsCreated = false;
	
	if (!DestroyInternal())
	{
		throw;
	}
}
