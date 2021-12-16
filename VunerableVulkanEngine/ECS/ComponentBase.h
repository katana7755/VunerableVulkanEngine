#pragma once

#include <cassert>
#include <typeindex>
#include <vector>
#include <algorithm>
#include <string>
#include "Entity.h"
#include "../rapidjson/document.h"
#include "../DebugUtility.h"

#define ECS_MAX_REGISTERED_COMPONENTTYPE_COUNT 512

namespace ECS
{
	typedef rapidjson::GenericObject<false, rapidjson::GenericValue<rapidjson::UTF8<>>> RapidJsonObject;

	struct ComponentBase
	{
	public:
		static const uint32_t INVALID_IDENTIFIER = 0xFFFFFFFF;
	};

	struct ComponentTypeInfo
	{
		std::string m_Name;
		size_t m_TypeHashCode;

		void* (*FuncCreateComponentArray)();
		void (*FuncDestroyComponentArray)(void* componentVectorPtr);
		void (*FuncAddComponentToArray)(void* componentVectorPtr);
		void (*FuncRemoveComponentFromArray)(void* componentVectorPtr, uint32_t index);
		void* (*FuncGetComponentFromArray)(void* componentVectorPtr, uint32_t index);
		void (*FuncSetComponentToArray)(void* componentVectorPtr, uint32_t index, void* componentDataPtr);
		void (*FuncDeserializeComponent)(RapidJsonObject& jsonEntityObject, const Entity& newEntity);
		void (*FuncSerializeComponent)(RapidJsonObject& jsonEntityObject, const Entity& newEntity);

		bool operator==(const std::type_index& typeIndex)
		{
			return m_TypeHashCode == typeIndex.hash_code();
		}
	};

	class ComponentTypeUtility
	{
	private:
		ComponentTypeUtility() {};

	public:
		static void UnregisterAllComponentTypes();

		template <class TComponentType>
		static void RegisterComponentType();

		static uint32_t GetComponentTypeCount();

		//template <class TComponentType>
		//static void UnregisterComponentType();

		template <class TComponentType>
		static uint32_t FindComponentIndex();

		static ComponentTypeInfo& GetComponentTypeInfo(uint32_t componentIndex);
		static void JsonDeserializeComponent(RapidJsonObject& entityObject, const Entity& newEntity, uint32_t componentIndex);
		static void JsonSerializeComponent(RapidJsonObject& entityObject, const Entity& newEntity, uint32_t componentIndex);

	private:
		static std::vector<ComponentTypeInfo> s_TypeInfoArray;
	};

	template <class TComponentType>
	void ComponentTypeUtility::RegisterComponentType()
	{
		// TODO: need to consider multex for supporting multi threading?
		static_assert(std::is_base_of<ComponentBase, TComponentType>::value, "this function should be called with a class derived by ECS::ComponentBase.");
		assert(s_TypeInfoArray.size() < ECS_MAX_REGISTERED_COMPONENTTYPE_COUNT);

		std::type_index typeIndex = typeid(TComponentType);
		size_t typeHashCode = typeIndex.hash_code();

		for (int i = 0; i < s_TypeInfoArray.size(); ++i)
		{
			if (s_TypeInfoArray[i].m_TypeHashCode == typeHashCode)
			{
				printf_console("a component type is not allowed to be registered more than once");

				throw;
			}
		}

		auto typeInfo = ComponentTypeInfo();
		typeInfo.m_Name = typeid(TComponentType).name();
		typeInfo.m_TypeHashCode = typeHashCode;
		typeInfo.FuncCreateComponentArray = []() {
			return (void*)(new std::vector<TComponentType>());
		};
		typeInfo.FuncDestroyComponentArray = [](void* componentVectorPtr) {
			assert(componentVectorPtr != NULL);

			delete componentVectorPtr;
		};
		typeInfo.FuncAddComponentToArray = [](void* componentVectorPtr) {
			assert(componentVectorPtr != NULL);

			auto& componentVectorRef = (*((std::vector<TComponentType>*)componentVectorPtr));
			componentVectorRef.emplace_back();
		};
		typeInfo.FuncRemoveComponentFromArray = [](void* componentVectorPtr, uint32_t index) {
			assert(componentVectorPtr != NULL);

			auto& componentVectorRef = (*((std::vector<TComponentType>*)componentVectorPtr));
			assert(index >= 0 && index < componentVectorRef.size());
			
			uint32_t lastIndex = componentVectorRef.size() - 1;

			if (index != lastIndex)
			{
				componentVectorRef[index] = componentVectorRef[lastIndex];
			}

			componentVectorRef.pop_back();
		};
		typeInfo.FuncGetComponentFromArray = [](void* componentVectorPtr, uint32_t index) {
			assert(componentVectorPtr != NULL);

			auto& componentVectorRef = (*(std::vector<TComponentType>*)componentVectorPtr);
			assert(index >= 0 && index < componentVectorRef.size());

			return (void*)(&componentVectorRef[index]);
		};
		typeInfo.FuncSetComponentToArray = [](void* componentVectorPtr, uint32_t index, void* componentDataPtr)
		{
			assert(componentVectorPtr != NULL);

			auto& componentVectorRef = (*((std::vector<TComponentType>*)componentVectorPtr));
			assert(index >= 0 && index < componentVectorRef.size());
			componentVectorRef[index] = (*(TComponentType*)componentDataPtr);
		};
		typeInfo.FuncDeserializeComponent = &TComponentType::JsonDeserialize;
		typeInfo.FuncSerializeComponent = &TComponentType::JsonSerialize;
		s_TypeInfoArray.push_back(typeInfo);
	}

	//template <class TComponentType>
	//void ComponentTypeUtility::UnregisterComponentType()
	//{
	//	// TODO: need to consider multex for supporting multi threading?
	//	static_assert(std::is_base_of<ComponentBase, TComponentType>::value, "this function should be called with a class derived by ECS::ComponentBase.");

	//	uint32_t foundIndex = FindComponentIndex<TComponentType>();
	//	uint32_t lastIndex = s_TypeInfoArray.size() - 1;

	//	if (foundIndex != lastIndex)
	//	{
	//		s_TypeInfoArray[foundIndex] = s_TypeInfoArray[lastIndex];
	//	}

	//	auto& typeInfo = s_TypeInfoArray[lastIndex];
	//	s_TypeInfoArray.pop_back();
	//}

	template <class TComponentType>
	uint32_t ComponentTypeUtility::FindComponentIndex()
	{
		// TODO: need to consider multex for supporting multi threading?
		static_assert(std::is_base_of<ComponentBase, TComponentType>::value, "this function should be called with a class derived by ECS::ComponentBase.");

		std::type_index typeIndex = typeid(TComponentType);
		auto foundIter = std::find(s_TypeInfoArray.begin(), s_TypeInfoArray.end(), typeIndex);
		assert(foundIter != s_TypeInfoArray.end());

		return foundIter - s_TypeInfoArray.begin();
	}
}