#pragma once

#include "../rapidjson/document.h"
#include "../ECS/Domain.h"

namespace ECS
{
	typedef rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> RapidJsonAllocator;

	void JsonSerizlieChunk(rapidjson::Value& jsonObject, RapidJsonAllocator& allocator, ECS::ComponentArrayChunk* chunkPtr);
}