#pragma once

#include "JsonSerialize.h"
#include "ComponentBase.h"
#include <string>
#include <cassert>

namespace ECS
{
	void JsonSerizlieChunk(RapidJsonObject& jsonObject, RapidJsonAllocator& allocator, ECS::ComponentArrayChunk* chunkPtr)
	{
		assert(chunkPtr != NULL);

		auto componentTypesKey = chunkPtr->m_ComponentTypesKey;
		std::string strBuffer = componentTypesKey.to_string();
		jsonObject.AddMember("m_BitField", rapidjson::Value().SetString(strBuffer.c_str(), strBuffer.size()), allocator);

		auto& entityArray = chunkPtr->m_EntityArray;
		jsonObject.AddMember("m_EntityArray", rapidjson::Value().SetArray(), allocator);

		auto& jsonEntityArray = jsonObject["m_EntityArray"].SetArray();

		for (uint32_t i = 0; i < entityArray.size(); ++i)
		{
			auto& entity = entityArray[i];
			auto& jsonEntityObject = jsonEntityArray[i].SetObject();
			auto entityObject = jsonEntityObject.GetObject();
			entityObject.AddMember("m_Identifier", rapidjson::Value().SetUint(entity.m_Identifier), allocator);

			for (uint32_t componentIndex = 0; componentIndex < ECS_MAX_REGISTERED_COMPONENTTYPE_COUNT; ++componentIndex)
			{
				if (componentTypesKey[componentIndex] == 0)
				{
					continue;
				}

				ComponentTypeUtility::JsonSerializeComponent(entityObject, entity, componentIndex);
			}
		}
	}
}