#pragma once
#include "vulkan/vulkan.h"
#include <limits>
#include <unordered_map>
#include <vector>
#include "../DebugUtility.h"

class VulkanGraphicsResourceBase
{
public:
	void Create();
	void Destroy();

protected:
	virtual bool CreateInternal() = 0;
	virtual bool DestroyInternal() = 0;

private:
	bool m_IsCreated;
};

template <class TResource>
class VulkanGraphicsResourceManagerBase
{
public:
	VulkanGraphicsResourceManagerBase()
		: m_UpcomingIdentifier(0)
	{
	}

public:
	TResource& GetResource(const size_t identifier);
	size_t AllocateResource();
	void ReleaseResource(const size_t identifier);

public:
	virtual void CreateResourcePhysically(const size_t identifier, const char* data, const unsigned int size) {};
	virtual void DestroyResourcePhysicially(const size_t identifier) {};

protected:
	std::vector<TResource> m_ResourceArray;
	std::vector<size_t> m_IdentifierArray;
	std::unordered_map<size_t, size_t> m_IdentifierToIndexMap;
	size_t m_UpcomingIdentifier;
};

template <class TResource>
TResource& VulkanGraphicsResourceManagerBase<TResource>::GetResource(const size_t identifier)
{
	if (m_IdentifierToIndexMap.find(identifier) == m_IdentifierToIndexMap.end())
	{
		printf_console("[VulkanGraphics] invalid try of getting gfx resource with id %d\n", identifier);

		throw;
	}

	size_t directIndex = m_IdentifierToIndexMap[identifier];

	return m_ResourceArray[directIndex];
}

template <class TResource>
size_t VulkanGraphicsResourceManagerBase<TResource>::AllocateResource()
{
	if (m_ResourceArray.size() >= ((size_t)-1))
	{
		printf_console("[VulkanGraphics] gfx resource array is full\n");

		throw;
	}

	size_t identifier = m_UpcomingIdentifier++;

	while (m_IdentifierToIndexMap.find(identifier) != m_IdentifierToIndexMap.end())
	{
		identifier = m_UpcomingIdentifier++;
	}

	size_t directIndex = m_ResourceArray.size();
	m_ResourceArray.push_back(TResource());
	m_IdentifierArray.push_back(identifier);
	m_IdentifierToIndexMap[identifier] = directIndex;

	return identifier;
}

template <class TResource>
void VulkanGraphicsResourceManagerBase<TResource>::ReleaseResource(const size_t identifier)
{
	if (m_IdentifierToIndexMap.find(identifier) == m_IdentifierToIndexMap.end())
	{
		printf_console("[VulkanGraphics] invalid try of release gfx resource with id %d\n", identifier);

		throw;
	}

	size_t releaseIndex = m_IdentifierToIndexMap[identifier];
	DestroyResourcePhysicially(identifier);

	size_t lastIndex = m_ResourceArray.size() - 1;

	if (releaseIndex > 0 && releaseIndex < lastIndex)
	{
		size_t lastIdentifier = m_IdentifierArray[lastIndex];
		m_ResourceArray[releaseIndex] = m_ResourceArray[lastIndex];
		m_IdentifierArray[releaseIndex] = m_IdentifierArray[lastIndex];
		m_IdentifierToIndexMap[lastIdentifier] = releaseIndex;
	}

	m_ResourceArray.erase(m_ResourceArray.begin() + lastIndex);
	m_IdentifierArray.erase(m_IdentifierArray.begin() + lastIndex);
	m_IdentifierToIndexMap.erase(identifier);
}