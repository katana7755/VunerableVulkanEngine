#pragma once

#include "../rapidjson/document.h"
#include "../ECS/Domain.h"

namespace ECS
{
	typedef rapidjson::GenericObject<false, rapidjson::GenericValue<rapidjson::UTF8<>>> RapidJsonObject;
	typedef rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> RapidJsonAllocator;

	void JsonSerizlieChunk(RapidJsonObject& jsonObject, RapidJsonAllocator& allocator, ECS::ComponentArrayChunk* chunkPtr);
}