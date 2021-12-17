#pragma once

#include "../rapidjson/document.h"
#include "Domain.h"

namespace ECS
{
	void JsonDeserizlieChunk(rapidjson::Value& jsonValue);
}