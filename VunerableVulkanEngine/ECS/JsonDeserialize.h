#pragma once

#include "../rapidjson/document.h"
#include "Domain.h"

namespace ECS
{
	typedef rapidjson::GenericObject<false, rapidjson::GenericValue<rapidjson::UTF8<>>> RapidJsonObject;

	void JsonDeserizlieChunk(RapidJsonObject& jsonObject);
}