#pragma once

#include <stdint.h>

namespace ECS
{
	struct Entity
	{
		friend class Domain;

		uint32_t m_Identifier;

	private:
		Entity() {}

	private:
		static Entity Create()
		{
			auto newEntity = Entity();
			newEntity.m_Identifier = s_Next_Allocate_Identifier++;
			return newEntity;
		}

	private:
		static uint32_t s_Next_Allocate_Identifier;

	public:
		bool operator==(const Entity& rhs)
		{
			return m_Identifier == rhs.m_Identifier;
		}

	public:
		static const uint32_t INVALID_IDENTIFIER = 0xFFFFFFFF;
	};
}