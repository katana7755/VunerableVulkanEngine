#pragma once

#include "../ECS/Domain.h"

namespace GameCore
{
	struct TransformComponent : ECS::ComponentBase
	{
	public:
		static void JsonDeserialize(ECS::RapidJsonObject& jsonEntityObject, const ECS::Entity& newEntity)
		{
		}

		static void JsonSerialize(ECS::RapidJsonObject& jsonEntityObject, const ECS::Entity& newEntity)
		{
		}
	};
}
