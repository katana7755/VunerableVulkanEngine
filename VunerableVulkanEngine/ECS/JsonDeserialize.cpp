#pragma once

#include "JsonDeserialize.h"
#include "ComponentBase.h"
#include <cassert>

namespace ECS
{
	void JsonDeserizlieChunk(RapidJsonObject& jsonObject)
	{
		assert(jsonObject["m_BitField"].IsString());
		assert(jsonObject["m_EntityArray"].IsArray());

		auto componentTypesKey = ECS::ComponentTypesKey(jsonObject["m_BitField"].GetString());
		auto entityObjectArray = jsonObject["m_EntityArray"].GetArray();

		for (uint32_t i = 0; i < entityObjectArray.Size(); ++i)
		{
			assert(entityObjectArray[i].IsObject());

			auto& jsonEntityObject = entityObjectArray[i];
			assert(jsonEntityObject.IsObject());

			auto entityObject = jsonEntityObject.GetObject();
			assert(entityObject["m_Identifier"].IsUint());

			uint32_t identifier = entityObject["m_Identifier"].GetUint();
			auto newEntity = ECS::Domain::CreateEntity(componentTypesKey, identifier);

			for (uint32_t componentIndex = 0; componentIndex < ECS_MAX_REGISTERED_COMPONENTTYPE_COUNT; ++componentIndex)
			{
				if (componentTypesKey[componentIndex] == 0)
				{
					continue;
				}

				ComponentTypeUtility::JsonDeserializeComponent(entityObject, newEntity, componentIndex);
			}
		}
	}
}