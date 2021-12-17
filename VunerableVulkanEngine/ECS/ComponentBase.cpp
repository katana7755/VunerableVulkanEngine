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

	void ComponentTypeUtility::JsonDeserializeComponent(rapidjson::Value& jsonValue, const Entity& newEntity, uint32_t componentIndex)
	{
		auto& typeInfo = GetComponentTypeInfo(componentIndex);
		typeInfo.FuncDeserializeComponent(jsonValue, newEntity);
	}

	void ComponentTypeUtility::JsonSerializeComponent(rapidjson::Value& jsonValue, const Entity& newEntity, uint32_t componentIndex)
	{
		auto& typeInfo = GetComponentTypeInfo(componentIndex);
		typeInfo.FuncSerializeComponent(jsonValue, newEntity);
	}
}
