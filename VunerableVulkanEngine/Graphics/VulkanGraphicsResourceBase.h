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

template <typename TResource, typename TInputData, typename TResourceKey>
class VulkanGraphicsResourceManagerBase
{
public:
	VulkanGraphicsResourceManagerBase()
		: m_UpcomingIdentifier(0)
	{
	}

public:
	TResource& GetResource(const size_t identifier);
	size_t AllocateIdentifier();
	void ReleaseIdentifier(const size_t identifier);
	void CreateResource(const size_t identifier, const TInputData& inputData, const TResourceKey& resourceKey);
	void DestroyResource(const size_t identifier);

protected:
	virtual TResource CreateResourcePhysically(const TInputData& inputData) { return TResource(); };
	virtual void DestroyResourcePhysicially(const TResource& resource) {};

protected:
	std::vector<TResource> m_ResourceArray;
	std::vector<size_t> m_IdentifierArray;
	std::unordered_map<size_t, size_t> m_IdentifierToIndexMap;
	std::unordered_map<size_t, size_t> m_IndexToIdentifierMap;
	size_t m_UpcomingIdentifier;
};

template <typename TResource, typename TInputData, typename TResourceKey>
TResource& VulkanGraphicsResourceManagerBase<TResource, TInputData, TResourceKey>::GetResource(const size_t identifier)
{
	if (m_IdentifierToIndexMap.find(identifier) == m_IdentifierToIndexMap.end())
	{
		printf_console("[VulkanGraphics] invalid try of getting gfx resource with id %d\n", identifier);

		throw;
	}

	size_t directIndex = m_IdentifierToIndexMap[identifier];

	if (directIndex < 0 || directIndex >= m_ResourceArray.size())
	{
		printf_console("[VulkanGraphics] invalid try of getting gfx resource with id %d\n", identifier);

		throw;
	}

	return m_ResourceArray[directIndex];
}

template <typename TResource, typename TInputData, typename TResourceKey>
size_t VulkanGraphicsResourceManagerBase<TResource, TInputData, TResourceKey>::AllocateIdentifier()
{
	if (m_IdentifierArray.size() >= ((size_t)-1))
	{
		printf_console("[VulkanGraphics] gfx resource array is full\n");

		throw;
	}

	size_t identifier = m_UpcomingIdentifier++;

	while (m_IdentifierToIndexMap.find(identifier) != m_IdentifierToIndexMap.end())
	{
		identifier = m_UpcomingIdentifier++;
	}

	m_IdentifierArray.push_back(identifier);
	m_IdentifierToIndexMap[identifier] = -1;

	return identifier;
}

template <typename TResource, typename TInputData, typename TResourceKey>
void VulkanGraphicsResourceManagerBase<TResource, TInputData, TResourceKey>::ReleaseIdentifier(const size_t identifier)
{
	if (m_IdentifierToIndexMap.find(identifier) == m_IdentifierToIndexMap.end())
	{
		printf_console("[VulkanGraphics] invalid try of release gfx resource with id %d\n", identifier);

		throw;
	}

	size_t directIndex = m_IdentifierToIndexMap[identifier];

	if (directIndex >= 0)
	{
		printf_console("[VulkanGraphics] invalid try of release gfx resource with id %d\n", identifier);

		throw;
	}

	auto iter = std::find(m_IdentifierArray.begin(), m_IdentifierArray.end(), identifier);

	if (iter != m_IdentifierArray.end())
	{
		m_IdentifierArray.erase(iter);
	}

	m_IdentifierToIndexMap.erase(identifier);
}

template <typename TResource, typename TInputData, typename TResourceKey>
void VulkanGraphicsResourceManagerBase<TResource, TInputData, TResourceKey>::CreateResource(const size_t identifier, const TInputData& inputData, const TResourceKey& resourceKey)
{
	size_t index = m_ResourceArray.size();
	m_IdentifierToIndexMap[identifier] = index;
	m_IndexToIdentifierMap[index] = identifier;
	m_ResourceArray.push_back(CreateResourcePhysically(inputData));
}

template <typename TResource, typename TInputData, typename TResourceKey>
void VulkanGraphicsResourceManagerBase<TResource, TInputData, TResourceKey>::DestroyResource(const size_t identifier)
{
	if (m_IdentifierToIndexMap.find(identifier) == m_IdentifierToIndexMap.end())
	{
		printf_console("[VulkanGraphics] invalid try of destroying gfx resource with id %d\n", identifier);

		throw;
	}

	size_t directIndex = m_IdentifierToIndexMap[identifier];

	if (directIndex < 0 || directIndex >= m_ResourceArray.size())
	{
		printf_console("[VulkanGraphics] invalid try of destroying gfx resource with id %d\n", identifier);

		throw;
	}

	DestroyResourcePhysicially(m_ResourceArray[directIndex]);

	size_t lastIndex = m_ResourceArray.size() - 1;
	size_t lastIdentifier = m_IndexToIdentifierMap[lastIndex];
	m_IdentifierToIndexMap[lastIdentifier] = directIndex;
	m_ResourceArray[directIndex] = m_ResourceArray[lastIndex];
	m_ResourceArray.erase(m_ResourceArray.begin() + lastIndex);
}

template <class TStruct>
struct HashForStructKey
{
	size_t operator()(const TStruct& key) const
	{
		const size_t keySize = sizeof(TStruct);
		char characterArray[keySize + 1];
		memcpy(characterArray, &key, keySize);
		characterArray[keySize] = 0;

		return std::hash<std::string>()(std::string(characterArray));
	}
};

template <class TStruct>
struct EqualToForStructKey
{
	bool operator()(const TStruct& lhs, const TStruct& rhs) const
	{
		const size_t keySize = sizeof(TStruct);
		return (memcmp(&lhs, &rhs, keySize) == 0);
	}
};