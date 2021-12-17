#pragma once

#include "../ECS/Domain.h"

namespace GameCore
{
	struct TransformComponent : ECS::ComponentBase
	{
	public:
		static void JsonDeserialize(rapidjson::Value& jsonValue, const ECS::Entity& newEntity)
		{
		}

		static void JsonSerialize(rapidjson::Value& jsonValue, const ECS::Entity& newEntity)
		{
		}
	};
}
