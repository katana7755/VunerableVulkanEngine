#pragma once

#include <cassert>
#include <vector>
#include <unordered_map>
#include <bitset>
#include <typeindex>
#include "Entity.h"
#include "ComponentBase.h"
#include "SystemBase.h"

#define ECS_MAX_COMPONENTARRAY_COUNT_IN_CHUNK 64

namespace ECS
{
	typedef std::bitset<ECS_MAX_COMPONENTARRAY_COUNT_IN_CHUNK>	ComponentTypesKey;

	struct ComponentArrayChunk
	{
		ComponentTypesKey				m_ComponentTypesKey;
		std::vector<Entity>				m_EntityArray;
		std::vector<ComponentTypeInfo>	m_ComponentTypeArray;
		std::vector<void*>				m_ComponentVectorArray; // std::vector of each component type...

		ComponentArrayChunk()
		{
		}

		ComponentArrayChunk(const ComponentTypesKey& componentTypesKey)
		{
			m_ComponentTypesKey = ComponentTypesKey(componentTypesKey);

			int componentCount = m_ComponentTypesKey.count();

			for (int i = 0; i < ECS_MAX_COMPONENTARRAY_COUNT_IN_CHUNK && componentCount > 0; ++i)
			{
				if (m_ComponentTypesKey[i] == false)
				{
					continue;
				}

				--componentCount;

				auto& typeInfo = ComponentTypeUtility::GetComponentTypeInfo(i);
				m_ComponentTypeArray.push_back(typeInfo);
				m_ComponentVectorArray.push_back(typeInfo.FuncCreateComponentArray());
			}
		}

		~ComponentArrayChunk()
		{
			for (int i = 0; i < m_ComponentVectorArray.size(); ++i)
			{
				auto& typeInfo = m_ComponentTypeArray[i];
				typeInfo.FuncDestroyComponentArray(m_ComponentVectorArray[i]);
			}
		}

		void AddEntity(const Entity& entity)
		{
			m_EntityArray.push_back(entity);
		}

		void RemoveEntity(uint32_t entityIndex)
		{
			uint32_t lastIndex = m_EntityArray.size() - 1;

			if (entityIndex != lastIndex)
			{
				m_EntityArray[entityIndex] = m_EntityArray[lastIndex];
			}

			m_EntityArray.pop_back();
		}
	};

	class Domain
	{
	private:
		Domain() {};

	public:
		template <class TComponentType>
		static void AddComponentTypeToKey(ComponentTypesKey& componentTypesKey);

		static Entity CreateEntity(const ComponentTypesKey& componentTypesKey);
		static void DestroyEntity(const Entity& entity);

		template <class TComponentType>
		static TComponentType GetComponent(const Entity& entity);

		template <class TComponentType>
		static void SetComponent(const Entity& entity, const TComponentType& componentData);

		template <class TSystemType>
		static uint32_t CreateSystem(uint32_t priority);

		static void DestroySystem(uint32_t systemIdentifier);
		static void ChangeSystemPriority(uint32_t systemIdentifier, uint32_t priority);
		static void ExecuteSystems();

		typedef void (*FuncForEachEntity)(const Entity&);
		static void ForEach(const ComponentTypesKey& componentTypesKey, FuncForEachEntity funcForEachEntity);

		static void Terminate();

	private:
		static std::unordered_map<uint32_t, ComponentTypesKey>				s_EntityToKeyMap;
		static std::unordered_map<ComponentTypesKey, ComponentArrayChunk*>	s_KeyToChunkPtrMap;
		static std::vector<SystemBase*>										s_SystemPtrArray;
		static bool															s_SystemsNeedToBeSorted;
	};

	template <class TComponentType>
	void Domain::AddComponentTypeToKey(ComponentTypesKey& componentTypesKey)
	{
		static_assert(std::is_base_of<ComponentBase, TComponentType>::value, "this function should be called with a class derived by ECS::ComponentBase.");

		uint32_t componentTypeIndex = ComponentTypeUtility::FindComponentIndex<TComponentType>();
		assert(componentTypeIndex != ComponentBase::INVALID_IDENTIFIER);

		componentTypesKey[componentTypeIndex] = true;
	}

	template <class TComponentType>
	TComponentType Domain::GetComponent(const Entity& entity)
	{
		static_assert(std::is_base_of<ComponentBase, TComponentType>::value, "this function should be called with a class derived by ECS::ComponentBase.");

		std::type_index typeIndex = typeid(TComponentType);
		auto keyIter = s_EntityToKeyMap.find(entity.m_Identifier);
		assert(keyIter != s_EntityToKeyMap.end());

		auto chunkPtrIter = s_KeyToChunkPtrMap.find(keyIter->second);
		assert(chunkPtrIter != s_KeyToChunkPtrMap.end());

		auto& chunk = *(chunkPtrIter->second);
		auto foundEntityIter = std::find(chunk.m_EntityArray.begin(), chunk.m_EntityArray.end(), entity);
		assert(foundEntityIter != chunk.m_EntityArray.end());

		uint32_t foundEntityIndex = foundEntityIter - chunk.m_EntityArray.begin();
		auto foundComponentTypeIter = std::find(chunk.m_ComponentTypeArray.begin(), chunk.m_ComponentTypeArray.end(), typeIndex);
		assert(foundComponentTypeIter != chunk.m_ComponentTypeArray.end());

		uint32_t foundComponentTypeIndex = foundComponentTypeIter - chunk.m_ComponentTypeArray.begin();
		auto* componentPtr = (TComponentType*)(chunk.m_ComponentTypeArray[foundComponentTypeIndex].FuncGetComponentFromArray(chunk.m_ComponentVectorArray[foundComponentTypeIndex], foundEntityIndex));

		return *componentPtr;
	}

	template <class TComponentType>
	void Domain::SetComponent(const Entity& entity, const TComponentType& componentData)
	{
		static_assert(std::is_base_of<ComponentBase, TComponentType>::value, "this function should be called with a class derived by ECS::ComponentBase.");

		std::type_index typeIndex = typeid(TComponentType);
		auto keyIter = s_EntityToKeyMap.find(entity.m_Identifier);
		assert(keyIter != s_EntityToKeyMap.end());

		auto chunkPtrIter = s_KeyToChunkPtrMap.find(keyIter->second);
		assert(chunkPtrIter != s_KeyToChunkPtrMap.end());

		auto& chunk = *(chunkPtrIter->second);
		auto foundEntityIter = std::find(chunk.m_EntityArray.begin(), chunk.m_EntityArray.end(), entity);
		assert(foundEntityIter != chunk.m_EntityArray.end());

		uint32_t foundEntityIndex = foundEntityIter - chunk.m_EntityArray.begin();
		auto foundComponentTypeIter = std::find(chunk.m_ComponentTypeArray.begin(), chunk.m_ComponentTypeArray.end(), typeIndex);
		assert(foundComponentTypeIter != chunk.m_ComponentTypeArray.end());

		uint32_t foundComponentTypeIndex = foundComponentTypeIter - chunk.m_ComponentTypeArray.begin();
		chunk.m_ComponentTypeArray[foundComponentTypeIndex].FuncSetComponentToArray(chunk.m_ComponentVectorArray[foundComponentTypeIndex], foundEntityIndex, (void*)(&componentData));
	}

	template <class TSystemType>
	uint32_t Domain::CreateSystem(uint32_t priority)
	{
		static_assert(std::is_base_of<SystemBase, TSystemType>::value, "this function should be called with a class derived by ECS::SystemBase.");

		auto* systemPtr = new TSystemType();
		uint32_t identifier = __COUNTER__;
		systemPtr->SetIdentifier(identifier);
		systemPtr->SetPriority(priority);
		systemPtr->OnInitialize();
		s_SystemPtrArray.push_back(systemPtr);
		s_SystemsNeedToBeSorted = true;
		
		return identifier;
	}
}