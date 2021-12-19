#pragma once

#include "../ECS/Domain.h"
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/transform.hpp"

namespace GameCore
{
	typedef rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> RapidJsonAllocator;

	struct TransformComponent : ECS::ComponentBase
	{
	public:
		uint32_t	m_Parent;
		glm::vec3	m_Position;
		glm::quat	m_Rotation;
		glm::vec3	m_Scale;

		TransformComponent()
		{
			m_Parent = ECS::Entity::INVALID_IDENTIFIER;
			m_Position = glm::vec3(0.0f, 0.0f, 0.0f);
			m_Rotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
			m_Scale = glm::vec3(1.0f, 1.0f, 1.0f);
		}

	public:
		static void JsonDeserialize(rapidjson::Value& jsonValue, const ECS::Entity& entity);
		static void JsonSerialize(rapidjson::Value& jsonValue, RapidJsonAllocator& allocator, const ECS::Entity& entity);
	};
}
