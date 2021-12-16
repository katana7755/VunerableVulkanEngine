#include "Domain.h"

namespace ECS
{
	bool Domain::IsChunkExist(const ComponentTypesKey& componentTypesKey)
	{
		assert(componentTypesKey.count() <= ECS_MAX_COMPONENTARRAY_COUNT_IN_CHUNK);

		return s_KeyToChunkPtrMap.find(componentTypesKey) != s_KeyToChunkPtrMap.end();
	}

	bool Domain::IsAliveEntity(const Entity& entity)
	{
		return s_EntityToKeyMap.find(entity.m_Identifier) != s_EntityToKeyMap.end();
	}

	const ComponentTypesKey& Domain::GetComponentTypesKey(const Entity& entity)
	{
		assert(s_EntityToKeyMap.find(entity.m_Identifier) != s_EntityToKeyMap.end());

		return s_EntityToKeyMap[entity.m_Identifier];
	}

	void Domain::CreateChunk(const ComponentTypesKey& componentTypesKey)
	{
		assert(s_KeyToChunkPtrMap.find(componentTypesKey) == s_KeyToChunkPtrMap.end());

		s_KeyToChunkPtrMap[componentTypesKey] = new ComponentArrayChunk(componentTypesKey);
	}

	void Domain::DestroyChunk(const ComponentTypesKey& componentTypesKey)
	{
		auto chunkPtrIter = s_KeyToChunkPtrMap.find(componentTypesKey);
		assert(chunkPtrIter != s_KeyToChunkPtrMap.end());

		auto& entityArray = chunkPtrIter->second->m_EntityArray;

		for (uint32_t i = 0; i < entityArray.size(); ++i)
		{
			auto keyIter = s_EntityToKeyMap.find(entityArray[i].m_Identifier);
			assert(keyIter != s_EntityToKeyMap.end());

			s_EntityToKeyMap.erase(keyIter);
		}

		delete chunkPtrIter->second;
		s_KeyToChunkPtrMap.erase(chunkPtrIter);
	}

	Entity Domain::CreateEntity(const ComponentTypesKey& componentTypesKey, uint32_t identifier)
	{
		assert(componentTypesKey.count() <= ECS_MAX_COMPONENTARRAY_COUNT_IN_CHUNK);
		
		auto chunkPtrIter = s_KeyToChunkPtrMap.find(componentTypesKey);

		if (chunkPtrIter == s_KeyToChunkPtrMap.end())
		{
			s_KeyToChunkPtrMap[componentTypesKey] = new ComponentArrayChunk(componentTypesKey);
			chunkPtrIter = s_KeyToChunkPtrMap.find(componentTypesKey);
		}

		auto newEntity = Entity::Create(identifier);

		// to avoid entity duplication when it is new creation
		if (identifier != Entity::INVALID_IDENTIFIER)
		{
			while (s_EntityToKeyMap.find(newEntity.m_Identifier) != s_EntityToKeyMap.end())
			{
				newEntity = Entity::Create(identifier);
			}
		}

		assert(s_EntityToKeyMap.find(newEntity.m_Identifier) == s_EntityToKeyMap.end());
		s_EntityToKeyMap[newEntity.m_Identifier] = componentTypesKey;

		auto& chunk = *(s_KeyToChunkPtrMap[componentTypesKey]);
		chunk.AddEntity(newEntity);
		
		for (int i = 0; i < chunk.m_ComponentTypeArray.size(); ++i)
		{
			chunk.m_ComponentTypeArray[i].FuncAddComponentToArray(chunk.m_ComponentVectorArray[i]);
		}

		return newEntity;
	}

	void Domain::DestroyEntity(const Entity& entity)
	{
		auto keyIter = s_EntityToKeyMap.find(entity.m_Identifier);
		assert(keyIter != s_EntityToKeyMap.end());

		auto chunkPtrIter = s_KeyToChunkPtrMap.find(keyIter->second);
		assert(chunkPtrIter != s_KeyToChunkPtrMap.end());

		auto& chunk = *(chunkPtrIter->second);
		auto findIter = std::find(chunk.m_EntityArray.begin(), chunk.m_EntityArray.end(), entity);
		assert(findIter != chunk.m_EntityArray.end());

		int findIndex = findIter - chunk.m_EntityArray.begin();
		chunk.RemoveEntity(findIndex);

		for (int i = 0; i < chunk.m_ComponentTypeArray.size(); ++i)
		{
			chunk.m_ComponentTypeArray[i].FuncRemoveComponentFromArray(chunk.m_ComponentVectorArray[i], findIndex);
		}

		if (chunk.m_EntityArray.size() <= 0)
		{
			delete chunkPtrIter->second;
			s_KeyToChunkPtrMap.erase(chunkPtrIter);
		}

		s_EntityToKeyMap.erase(keyIter);
	}

	void Domain::DestroySystem(uint32_t systemIdentifier)
	{
		auto findIter = std::find_if(s_SystemPtrArray.begin(), s_SystemPtrArray.end(), [=](SystemBase* systemPtr) {
			return systemPtr->GetIdentifier() == systemIdentifier;
			});
		assert(findIter != s_SystemPtrArray.end());

		delete (*findIter);
		s_SystemPtrArray.erase(findIter);
	}

	void Domain::ChangeSystemPriority(uint32_t systemIdentifier, uint32_t priority)
	{
		auto findIter = std::find_if(s_SystemPtrArray.begin(), s_SystemPtrArray.end(), [=](SystemBase* systemPtr) {
			return systemPtr->GetIdentifier() == systemIdentifier;
			});
		assert(findIter != s_SystemPtrArray.end());
		(*findIter)->SetPriority(priority);
		s_SystemsNeedToBeSorted = true;
	}

	void Domain::ExecuteSystems()
	{
		if (s_SystemsNeedToBeSorted)
		{
			std::sort(s_SystemPtrArray.begin(), s_SystemPtrArray.end(), [](SystemBase* lhs, SystemBase* rhs) {
				return lhs->GetPriority() > rhs->GetPriority();
				});
			s_SystemsNeedToBeSorted = false;
		}

		for (auto* systemPtr : s_SystemPtrArray)
		{
			systemPtr->OnExecute();
		}
	}

	void Domain::Terminate()
	{
		for (auto chunkPtrPair : s_KeyToChunkPtrMap)
		{
			delete chunkPtrPair.second;
		}

		s_EntityToKeyMap.clear();
		s_KeyToChunkPtrMap.clear();

		for (auto systemPtr : s_SystemPtrArray)
		{
			delete systemPtr;
		}

		s_SystemPtrArray.clear();
		s_SystemsNeedToBeSorted = false;
		ComponentTypeUtility::UnregisterAllComponentTypes();
	}

	std::unordered_map<uint32_t, ComponentTypesKey>	Domain::s_EntityToKeyMap;
	std::unordered_map<ComponentTypesKey, ComponentArrayChunk*> Domain::s_KeyToChunkPtrMap;
	std::vector<SystemBase*> Domain::s_SystemPtrArray;
	bool Domain::s_SystemsNeedToBeSorted = false;
}
