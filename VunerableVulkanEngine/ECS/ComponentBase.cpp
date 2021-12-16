#include "ComponentBase.h"

namespace ECS
{
	std::vector<ComponentTypeInfo> ComponentTypeUtility::s_TypeInfoArray;

	void ComponentTypeUtility::UnregisterAllComponentTypes()
	{
		s_TypeInfoArray.clear();
	}

	ComponentTypeInfo& ComponentTypeUtility::GetComponentTypeInfo(uint32_t componentIndex)
	{
		assert(componentIndex >= 0 && componentIndex < s_TypeInfoArray.size());

		return s_TypeInfoArray[componentIndex];
	}

	uint32_t ComponentTypeUtility::GetComponentTypeCount()
	{
		return s_TypeInfoArray.size();
	}

	void ComponentTypeUtility::JsonDeserializeComponent(RapidJsonObject& jsonEntityObject, const Entity& newEntity, uint32_t componentIndex)
	{
		auto& typeInfo = GetComponentTypeInfo(componentIndex);
		typeInfo.FuncDeserializeComponent(jsonEntityObject, newEntity);
	}

	void ComponentTypeUtility::JsonSerializeComponent(RapidJsonObject& jsonEntityObject, const Entity& newEntity, uint32_t componentIndex)
	{
		auto& typeInfo = GetComponentTypeInfo(componentIndex);
		typeInfo.FuncSerializeComponent(jsonEntityObject, newEntity);
	}
}
