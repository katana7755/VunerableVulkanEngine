#include "Domain.h"

namespace ECS
{
	ComponentTypesKey Domain::CreateComponentTypesKey()
	{
		return ComponentTypesKey();
	}

	Entity Domain::CreateEntity(const ComponentTypesKey& componentTypesKey)
	{
		assert(componentTypesKey.count() < ECS_MAX_COMPONENTARRAY_COUNT_IN_CHUNK);
		
		auto chunkPtrIter = s_KeyToChunkPtrMap.find(componentTypesKey);

		if (chunkPtrIter == s_KeyToChunkPtrMap.end())
		{
			s_KeyToChunkPtrMap[componentTypesKey] = new ComponentArrayChunk(componentTypesKey);
			chunkPtrIter = s_KeyToChunkPtrMap.find(componentTypesKey);
		}

		auto newEntity = Entity::Create();
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
		DestroyEntityByIdentifier(entity.m_Identifier);
	}

	void Domain::DestroyEntityByIdentifier(const uint32_t& entityIdentifier)
	{
		auto keyIter = s_EntityToKeyMap.find(entityIdentifier);
		assert(keyIter != s_EntityToKeyMap.end());

		auto chunkPtrIter = s_KeyToChunkPtrMap.find(keyIter->second);
		assert(chunkPtrIter != s_KeyToChunkPtrMap.end());

		auto& chunk = *(chunkPtrIter->second);
		auto findIter = std::find(chunk.m_EntityIdentifierArray.begin(), chunk.m_EntityIdentifierArray.end(), entityIdentifier);
		assert(findIter != chunk.m_EntityIdentifierArray.end());

		int findIndex = findIter - chunk.m_EntityIdentifierArray.begin();
		chunk.RemoveEntity(findIndex);

		for (int i = 0; i < chunk.m_ComponentTypeArray.size(); ++i)
		{
			chunk.m_ComponentTypeArray[i].FuncRemoveComponentFromArray(chunk.m_ComponentVectorArray[i], findIndex);
		}

		if (chunk.m_EntityIdentifierArray.size() <= 0)
		{
			delete chunkPtrIter->second;
			s_KeyToChunkPtrMap.erase(chunkPtrIter);
		}

		s_EntityToKeyMap.erase(keyIter);
	}

	void Domain::DestroyAllEntities()
	{
		for (auto chunkPtrPair : s_KeyToChunkPtrMap)
		{
			delete chunkPtrPair.second;
		}

		s_EntityToKeyMap.clear();
		s_KeyToChunkPtrMap.clear();

		//std::vector<uint32_t> identifierArray;

		//for (auto pair : s_EntityToKeyMap)
		//{
		//	identifierArray.push_back(pair.first);
		//}

		//for (auto identifier : identifierArray)
		//{
		//	DestroyEntityByIdentifier(identifier);
		//}
	}

	std::unordered_map<uint32_t, ComponentTypesKey>	Domain::s_EntityToKeyMap;
	std::unordered_map<ComponentTypesKey, ComponentArrayChunk*> Domain::s_KeyToChunkPtrMap;
}
