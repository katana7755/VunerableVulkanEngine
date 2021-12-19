#pragma once

#include "JsonSerialize.h"
#include "ComponentBase.h"
#include <string>
#include <cassert>
#include "../rapidjson/stringbuffer.h"
#include "../rapidjson/encodings.h"

namespace ECS
{
	std::string g_StrBuffer;

	void JsonSerizlieChunk(rapidjson::Value& jsonValue, RapidJsonAllocator& allocator, ECS::ComponentArrayChunk* chunkPtr)
	{
		assert(chunkPtr != NULL);

		auto componentTypesKey = chunkPtr->m_ComponentTypesKey;
		g_StrBuffer = componentTypesKey.to_string();
		jsonValue.AddMember("m_BitField", rapidjson::Value().SetString(g_StrBuffer.c_str(), g_StrBuffer.size()), allocator);

		auto& entityArray = chunkPtr->m_EntityArray;
		jsonValue.AddMember("m_EntityArray", rapidjson::Value().SetArray(), allocator);

		auto& jsonEntityArray = jsonValue["m_EntityArray"].SetArray();

		for (uint32_t i = 0; i < entityArray.size(); ++i)
		{
			auto& entity = entityArray[i];
			auto newJsonValue = rapidjson::Value();
			newJsonValue.SetObject();
			newJsonValue.AddMember("m_Identifier", rapidjson::Value().SetUint(entity.m_Identifier), allocator);

			int componentCount = componentTypesKey.count();

			for (int componentIndex = 0; componentIndex < ECS_MAX_REGISTERED_COMPONENTTYPE_COUNT && componentCount > 0; ++componentIndex)
			{
				if (componentTypesKey[componentIndex] == 0)
				{
					continue;
				}

				--componentCount;
				ComponentTypeUtility::JsonSerializeComponent(newJsonValue, allocator, entity, componentIndex);
			}

			jsonEntityArray.PushBack(newJsonValue, allocator);
		}
	}
}