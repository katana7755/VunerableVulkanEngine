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
}
